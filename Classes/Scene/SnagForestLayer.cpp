#include "SnagForestLayer.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
#include "PolySprite.h"
#else
#include "Tools/PolySprite.h"
#endif
USING_NS_CC;
USING_NS_CC_EXT;

SnagForestLayer::SnagForestLayer()
	: m_ballAngle(-1.0)
	, m_ball(NULL)
	, m_arrow(NULL)
	, m_littleGameSlot(NULL)
	, m_isBallGoingUp(true)
	, m_isLittleGameStart(false)
	, m_devil(NULL)
	, m_emitter(NULL)
	, m_listener(NULL)
	, m_resLoadingProgresser(NULL)
	, m_loadingVar(1.0f)
	, m_cellside(0.0f)
	, m_loadBg(NULL)
{
	m_slotPos = new std::vector<Point>();
}

SnagForestLayer::~SnagForestLayer()
{
	_eventDispatcher->removeEventListener(m_listener);
	m_cellMap.clear();
	CC_SAFE_DELETE(m_slotPos);
}

void SnagForestLayer::setPhyWorld(PhysicsWorld* world)
{
	this->m_physicsWorld = world;
}

bool SnagForestLayer::initWithEntryID(int entryId)
{
	//log("updateProgresser    %d", time(NULL));
	initResourcesWithProgresser();
	interactionSubscribe();
	this->scheduleUpdate();

	//add listener for sreen touch
	m_listener = EventListenerTouchOneByOne::create();
	m_listener->setSwallowTouches(true);
	m_listener->onTouchBegan = CC_CALLBACK_2(SnagForestLayer::TouchBegan, this);
	m_listener->onTouchMoved = CC_CALLBACK_2(SnagForestLayer::TouchMoved, this);
	m_listener->onTouchEnded = CC_CALLBACK_2(SnagForestLayer::TouchEnded, this);
	m_listener->setEnabled(false);
	_eventDispatcher->addEventListenerWithFixedPriority(m_listener, -1);

	return true;
}

void SnagForestLayer::update(float dt)
{
	// remove ball when it arrive the bottom of layer
	if (m_ball != NULL && m_ball->getPositionY() <= (m_ball->getBallSize().height / 2 + c_ballHeightStop))
	{
		this->removeChild(m_ball);
		m_ball = NULL;
		m_isBallGoingUp = true;
		if (!m_listener->isEnabled())
		{
			m_listener->setEnabled(true);
		}
		m_emitter->setVisible(false);
		if (m_devil != NULL)
		{
			m_devil->setDevilMaxIndexInCurrent(m_devil->getDevilMaxIndexInCurrent() + m_devil->getDevilPosCnt());
			m_devil->setDevilPosCnt(0);
			NotificationCenter::getInstance()->postNotification(MsgTypeForObserver::c_DevilPosPush, m_devil);
		}
	}
	// the ball is falling and drawing the routed path
	else if (m_ball != NULL && !m_isBallGoingUp)
	{
		if (m_devil != NULL && isCollidedWithBall(m_ball, m_devil))
		{
			triggerDevil();
			removeDevil();
		}
		routingDetection();
	}

	if (m_ball != NULL && m_littleGameSlot != NULL && isCollidedWithBall(m_ball, m_littleGameSlot))
	{
		if (!m_isLittleGameStart)
		{
			NotificationCenter::getInstance()->postNotification(MsgTypeForObserver::c_BalloonStart, NULL);
			pause();
			m_isLittleGameStart = true;
		}
	}
}


void SnagForestLayer::updateProgresser(float dt)
{
	float loadingVar = m_resLoadingProgresser->getPercentage();
	if (m_resLoadingProgresser != NULL && loadingVar < 100.0f)
	{
		m_resLoadingProgresser->setPercentage(loadingVar + 1.0f);
	}
	else if (loadingVar >= 100.0f)
	{
		this->unschedule(schedule_selector(SnagForestLayer::updateProgresser));
		//m_resLoadingProgresser->setPercentage(100.0f);

		this->removeChild(m_resLoadingProgresser);
		this->removeChild(m_loadBg);
		if (m_listener != NULL && !m_listener->isEnabled())
		{
			m_listener->setEnabled(true);
		}
	}
}

void SnagForestLayer::ballRising(float dt)
{
	if (m_ball != NULL && !m_listener->isEnabled())
	{
		Point expect;
		// the position of X is depend on m_ballAngle, if m_ballAngle is positive, the X should increase otherwise is should be decrease
		float expectX = BALL_LAUNCH_SPEED*tan(CC_DEGREES_TO_RADIANS(m_ballAngle));
		if (m_isBallGoingUp)
		{
			// ball is rising, so the Y need be positive
			expect = Point(expectX, BALL_LAUNCH_SPEED);
			// if ball arrived the top of windows height, let ball down
			m_isBallGoingUp = m_ball->getPositionY() < (m_winSize.height - m_ball->getBallSize().height / 2) ? true : false;
		}
		else
		{
			// ball is falling, so the Y need be negative
			expect = Point(expectX, -BALL_LAUNCH_SPEED);
		}
		// moving ball
		m_ball->setPosition(m_ball->getPosition() + expect);

		// when ball is reach the indicate position, make ball falling in physical world
		if (!m_isBallGoingUp && m_ball->getPositionY() < m_winSize.height - c_ballHeightStart)
		{
			m_ball->getPhysicsBody()->setDynamic(true);
			// stop rising
			this->unschedule(schedule_selector(SnagForestLayer::ballRising));
		}
	}
}


bool SnagForestLayer::TouchBegan(Touch* touch, Event* event)
{
	schedule(schedule_selector(SnagForestLayer::ballRising));
	if (m_ball != NULL)
	{
		this->removeChild(m_ball);
		m_ball = NULL;
	}

	// create ball and set position of ball on arrow
	m_ball = Ball::create();
	m_ball->bindSprite(Sprite::createWithSpriteFrame(
		SpriteFrameCache::getInstance()->getSpriteFrameByName("ball.png")
		));
	m_ball->setBallSize(m_ball->getSprite()->getContentSize());
	PhysicsMaterial pm;
	pm.density = 0.0f;
	pm.friction = 2.0f*CCRANDOM_0_1();
	pm.restitution = 0.9f;
	auto body = PhysicsBody::createCircle(m_ball->getBallSize().width / 2, pm);

	// set to static, otherwise it will collide snags when launch the ball
	body->setDynamic(false);
	m_ball->setPhysicsBody(body);
	CC_ASSERT(m_arrow != NULL);

	//set position of ball on arrow
	m_ball->setPosition(m_arrow->getPosition());

	this->addChild(m_ball, Z_ORDER_MAX);

	//show arrow
	m_arrow->setRotation(0);
	m_arrow->setVisible(true);
	return true;

}
void SnagForestLayer::TouchMoved(Touch* touch, Event* event)
{
	Point start = touch->getStartLocation();

	Point afterStart = touch->getLocation();

	// set the angle of ball (m_arrow) to indicate the direction of launching ball
	if (afterStart.y - start.y < m_winSize.height && afterStart.x - start.x != 0)
	{
		// get the radian by atanf function
		m_ballAngle = atanf((afterStart.x - start.x) / abs(afterStart.y - m_ball->getPositionY()));
		// convert radian to degrees
		float upBallRotation = (float)CC_RADIANS_TO_DEGREES(m_ballAngle);
		if (upBallRotation < 0)
		{
			// set negative degree is not less than -BALL_LAUNCH_ROTATION
			upBallRotation = upBallRotation < -BALL_LAUNCH_ROTATION ? -BALL_LAUNCH_ROTATION : upBallRotation;
		}
		else if (upBallRotation > 0)
		{
			// set positive degree is not more than BALL_LAUNCH_ROTATION
			upBallRotation = upBallRotation > BALL_LAUNCH_ROTATION ? BALL_LAUNCH_ROTATION : upBallRotation;
		}
		// set angle of both ball and arrow when TouchMoved
		m_ballAngle = upBallRotation;
		m_ball->setRotation(m_ballAngle);
		m_arrow->setRotation(m_ballAngle);
	}

}
void SnagForestLayer::TouchEnded(Touch* touch, Event* event)
{
	m_ballAngle = m_ball->getRotation();
	// hide arrow
	m_arrow->setVisible(false);
	m_listener->setEnabled(false);
}


/*private function*/

/* === Initial Scene ===*/
void SnagForestLayer::initResourcesWithProgresser()
{
	m_winSize = Director::getInstance()->getWinSize();
	m_cellside = sqrt((m_winSize.width / 6)*(m_winSize.width / 6) / 2);
	m_winX = m_winSize.width - c_radius * 2;
	initProgresser();

	SpriteFrameCache::getInstance()->addSpriteFramesWithFile("SnagForestScene.plist", "SnagForestScene.png");


	/*initialize stuff for this Scene*/

	initMap();
	initSnags();
	initSlots();
	initCells();
	initArrow();
	initParticleFire();
}

void SnagForestLayer::initProgresser()
{
	Point p = Point(m_winSize.width / 2, m_winSize.height / 4 + 50);
	m_loadBg = Sprite::create("slider_bar.png");//base map of progress bar
	m_loadBg->setScale(300);
	m_loadBg->setPosition(p);
	this->addChild(m_loadBg, Z_ORDER_MAX + 1);

	m_resLoadingProgresser = ProgressTimer::create(Sprite::create("silder_progressBar.png"));
	CC_ASSERT(m_resLoadingProgresser != NULL);
	m_resLoadingProgresser->setType(ProgressTimer::Type::BAR);
	m_resLoadingProgresser->setPosition(p);  // set progress bar be same with m_loadBg
	m_resLoadingProgresser->setMidpoint(Point(0, 0));
	m_resLoadingProgresser->setBarChangeRate(Point(1, 0));
	m_resLoadingProgresser->setPercentage(0);
	this->addChild(m_resLoadingProgresser, Z_ORDER_MAX + 1);
	this->schedule(schedule_selector(SnagForestLayer::updateProgresser));
}

void SnagForestLayer::initMap()
{
	auto edgeBody = PhysicsBody::createEdgeBox(this->m_winSize, PHYSICSBODY_MATERIAL_DEFAULT, 1);

	auto bg_Sprite = Sprite::create("SnagForestScene_bg.jpg");
	bg_Sprite->setPosition(Point(m_winSize.width / 2, m_winSize.height / 2));
	bg_Sprite->setPhysicsBody(edgeBody);
	this->addChild(bg_Sprite, Z_ORDER_ZERO);

	auto fg_Sprite = Sprite::create("SnagForestScene_fg.jpg");
	fg_Sprite->setPosition(Point(m_winSize.width / 2, c_snagHeightStart - fg_Sprite->getContentSize().height / 2 + c_radius));
	this->addChild(fg_Sprite, Z_ORDER_ZERO);
}

void SnagForestLayer::initSnags()
{
	auto snags = SpriteBatchNode::createWithTexture(SpriteFrameCache::getInstance()->getSpriteFrameByName("snag.png")->getTexture());
	this->addChild(snags, Z_ORDER_FOUR);
	PhysicsMaterial pm;
	pm.density = 1.0f;
	pm.restitution = 0.5f;
	for (int column = 0; column < 7; ++column)//columns
	{
		for (int row = 0; row < 13; ++row)//rows
		{
			auto snag = Sprite::createWithSpriteFrame(
				SpriteFrameCache::getInstance()->getSpriteFrameByName("snag.png")
				);
			pm.friction = 0.1f*CCRANDOM_0_1();
			if (row % 2 == 1)
			{
				snag->setPosition(Point(m_winX / 6 * column + c_radius, (c_snagHeightStart - (m_winX / 6 / 2)*row)));
			}
			else
			{
				snag->setPosition(Point(m_winX / 6 / 2 + m_winX / 6 * column + c_radius, (c_snagHeightStart - (m_winX / 6 / 2)*row)));
			}
			auto body = PhysicsBody::createCircle(snag->getContentSize().width / 2, pm);
			body->setDynamic(false);
			snag->setPhysicsBody(body);
			snags->addChild(snag);
		}
	}
}

void SnagForestLayer::initSlots()
{
	auto slots = SpriteBatchNode::createWithTexture(SpriteFrameCache::getInstance()->getSpriteFrameByName("slot.png")->getTexture());
	this->addChild(slots, Z_ORDER_MAX);
	for (int i = 0; i < 8; ++i)
	{
		auto slot = Sprite::createWithSpriteFrame(
			SpriteFrameCache::getInstance()->getSpriteFrameByName("slot.png")
			);
		auto body = PhysicsBody::createBox(slot->getContentSize());
		body->setDynamic(false);
		slot->setPhysicsBody(body);

		//Node warning : This node has a physics body, the anchor must be in the middle, you cann't change this to other value.
		//slot->setAnchorPoint(Point::ANCHOR_MIDDLE_BOTTOM);

		slot->setPosition(Point(m_winX / 9 * (i + 1) + slot->getContentSize().width / 2, slot->getContentSize().height / 2));
		slots->addChild(slot);
	}
	m_littleGameSlot = Sprite::create("littlegameslot.png");
	m_littleGameSlot->setScale(0.4f);

	float gameslotY = m_littleGameSlot->getContentSize().height/2*0.4f;
	//gameslotY=960.0f;

	m_slotPos->push_back(Point(m_winSize.width / 2,  gameslotY));
	for (int idx = 1; idx < 5; ++idx)
	{
		m_slotPos->push_back(Point(m_winSize.width / 2 - m_winX / 9 * idx,  gameslotY));
		m_slotPos->push_back(Point(m_winSize.width / 2 + m_winX / 9 * idx,  gameslotY));
	}
	m_slotPosIdx = MsgTypeForObserver::getRand(0,m_slotPos->size()-1);
	m_littleGameSlot->setPosition(m_slotPos->at(m_slotPosIdx));
	this->addChild(m_littleGameSlot, Z_ORDER_THREE);
}

void SnagForestLayer::initCells()
{
	auto cellPos = Point(0, 0);
	int index[] = { 0, 1, 2, 0, 3, 2 };
	for (int row = 0; row < 13; ++row)//rows
	{
		Vector<Ref*> cellVec;
		for (int column = 0; column < 7; ++column)//columns
		{
			if (row % 2 == 0)
			{
				cellPos = Point(m_winX / 6 * column + c_radius, (c_snagHeightStart - (m_winX / 6 / 2)*row));
			}
			else
			{
				cellPos = Point(m_winX / 6 / 2 + m_winX / 6 * column + c_radius, (c_snagHeightStart - (m_winX / 6 / 2)*row));
			}
			Point p[] = {
				Point((cellPos.x - m_winSize.width / 6 / 2) / m_winSize.width, (m_winSize.height - cellPos.y) / m_winSize.height),
				Point(cellPos.x / m_winSize.width, (m_winSize.height - cellPos.y + m_winSize.width / 6 / 2) / m_winSize.height),
				Point((cellPos.x + m_winSize.width / 6 / 2) / m_winSize.width, (m_winSize.height - cellPos.y) / m_winSize.height),
				Point(cellPos.x / m_winSize.width, (m_winSize.height - cellPos.y - m_winSize.width / 6 / 2) / m_winSize.height)
			};
			auto cell = PolySprite::create("SnagForestScene_bg.jpg", p, 4, index);
			//cell->setPosition(Point(cell->getAnchorPoint().x*m_winSize.width, cell->getAnchorPoint().y*m_winSize.height));
			cell->setPosition(cellPos);
			cell->setVisible(false);
			this->addChild(cell, Z_ORDER_ONE);
			cellVec.pushBack(cell);
		}
		m_cellMap.insert(std::pair<int, Vector<Ref*>>(row, cellVec));
	}
}

void SnagForestLayer::initArrow()
{
	m_arrow = Sprite::createWithSpriteFrame(
		SpriteFrameCache::getInstance()->getSpriteFrameByName("slot.png")
		);
	m_arrow->setAnchorPoint(Point::ANCHOR_MIDDLE_BOTTOM);
	m_arrow->setPosition(Point(m_winSize.width / 2 - CCRANDOM_0_1(), c_ballHeightBegin));
	m_arrow->setVisible(false);
	this->addChild(m_arrow, Z_ORDER_MAX + 1);
}

// Particle
void SnagForestLayer::initParticleFire()
{
	m_emitter = ParticleFire::create();
	m_emitter->retain();
	this->addChild(m_emitter, Z_ORDER_MAX);
	m_emitter->setTexture(Director::getInstance()->getTextureCache()->addImage("fire.png"));
	m_emitter->setEmissionRate(800);
	m_emitter->setSpeed(120);
	m_emitter->setSpeedVar(0);
	m_emitter->setContentSize(m_winSize);
	m_emitter->setLifeVar(0.3f);
	m_emitter->setLife(0.2f);
	m_emitter->setDuration(-1);
	m_emitter->setStartSize(20.0f);
	m_emitter->setStartSizeVar(50.0f);
	m_emitter->setEndSize(ParticleSystem::START_SIZE_EQUAL_TO_END_SIZE);
	m_emitter->setBlendAdditive(true);
	m_emitter->setVisible(false);
}

/* === Ball Action ===*/
void SnagForestLayer::routingDetection()
{
	if (m_ball->getPositionY() > c_snagHeightStart - m_winX / 6 / 2 && m_ball->getPositionY() <= c_snagHeightStart + m_winX / 6 / 2)
	{
		showCells(0);
	}
	else if (m_ball->getPositionY() > c_snagHeightStart - (m_winX / 6) && m_ball->getPositionY() <= c_snagHeightStart)
	{
		showCells(1);
	}
	else if (m_ball->getPositionY() > c_snagHeightStart - (m_winX / 6 * 1.5) && m_ball->getPositionY() <= c_snagHeightStart - (m_winX / 6))
	{
		showCells(2);
	}
	else if (m_ball->getPositionY() > c_snagHeightStart - (m_winX / 6 * 2) && m_ball->getPositionY() <= c_snagHeightStart - (m_winX / 6 * 1.5))
	{
		showCells(3);
	}
	else if (m_ball->getPositionY() > c_snagHeightStart - (m_winX / 6 * 2.5) && m_ball->getPositionY() <= c_snagHeightStart - (m_winX / 6 * 2))
	{
		showCells(4);
	}
	else if (m_ball->getPositionY() > c_snagHeightStart - (m_winX / 6 * 3) && m_ball->getPositionY() <= c_snagHeightStart - (m_winX / 6 * 2.5))
	{
		showCells(5);
	}
	else if (m_ball->getPositionY() > c_snagHeightStart - (m_winX / 6 * 3.5) && m_ball->getPositionY() <= c_snagHeightStart - (m_winX / 6 * 3))
	{
		showCells(6);
	}
	else if (m_ball->getPositionY() > c_snagHeightStart - (m_winX / 6 * 4) && m_ball->getPositionY() <= c_snagHeightStart - (m_winX / 6 * 3.5))
	{
		showCells(7);
	}
	else if (m_ball->getPositionY() > c_snagHeightStart - (m_winX / 6 * 4.5) && m_ball->getPositionY() <= c_snagHeightStart - (m_winX / 6 * 4))
	{
		showCells(8);
	}
	else if (m_ball->getPositionY() > c_snagHeightStart - (m_winX / 6 * 5) && m_ball->getPositionY() <= c_snagHeightStart - (m_winX / 6 * 4.5))
	{
		showCells(9);
	}
	else if (m_ball->getPositionY() > c_snagHeightStart - (m_winX / 6 * 5.5) && m_ball->getPositionY() <= c_snagHeightStart - (m_winX / 6 * 5))
	{
		showCells(10);
	}
	else if (m_ball->getPositionY() > c_snagHeightStart - (m_winX / 6 * 6) && m_ball->getPositionY() <= c_snagHeightStart - (m_winX / 6 * 5.5))
	{
		showCells(11);
	}
	else if (m_ball->getPositionY() > c_snagHeightStart - (m_winX / 6 * 6.5) && m_ball->getPositionY() <= c_snagHeightStart - (m_winX / 6 * 6))
	{
		showCells(12);
	}

}

void SnagForestLayer::showCells(unsigned int indexOfCellArr)
{
	Ref* obj = NULL;
	Sprite* cell = NULL;
	auto cellVec = m_cellMap.at(indexOfCellArr);
	for (auto& element : cellVec)
	{
		cell = (Sprite*)element;
		if (isCollidedWithBall(m_ball, cell))
		{
			if (!cell->isVisible())
			{
				auto cellPos = cell->getPosition();
				cell->setVisible(true);
				if (m_emitter != NULL)
				{
					m_emitter->setPosition(cellPos);
					m_emitter->setVisible(true);
				}
				if (m_devil != NULL && indexOfCellArr != 0 && indexOfCellArr != 12)
				{
					m_devil->getDevilPosVec()->push_back(cellPos);
					m_devil->setDevilPosCnt(m_devil->getDevilPosCnt() + 1);
				}
			}
		}
	}
}

bool SnagForestLayer::isCollidedWithBall(Ball* ball, Node *node)
{
	if (ball->getSprite() != NULL)
	{
		auto entityRrct = ball->getBoundingBox();
		auto nodePos = node->getPosition();
		return entityRrct.containsPoint(nodePos);
	}

	return false;
}


/* === Devil Action ===*/
void SnagForestLayer::handleDevil(Ref* pData)
{
	m_devil = (Devil*)pData;
	m_devil->retain();
	NotificationCenter::getInstance()->removeObserver(this, MsgTypeForObserver::c_DevilPosUpdate);
}

void SnagForestLayer::handleDevilStop(Ref* pData)
{
	removeDevil();
	//recover();
	NotificationCenter::getInstance()->removeObserver(this, MsgTypeForObserver::c_DevilFightingStop);
	NotificationCenter::getInstance()->postNotification(MsgTypeForObserver::c_BattleStart, NULL);
}

void SnagForestLayer::handleBattleStop(Ref* pData)
{
	recover();
	NotificationCenter::getInstance()->removeObserver(this, MsgTypeForObserver::c_BattleStop);
}

void SnagForestLayer::interactionSubscribe()
{
	NotificationCenter::getInstance()->addObserver(
		this,
		callfuncO_selector(SnagForestLayer::handleDevil),
		MsgTypeForObserver::c_DevilPosUpdate,
		NULL);

	NotificationCenter::getInstance()->addObserver(
		this,
		callfuncO_selector(SnagForestLayer::handleDevilStop),
		MsgTypeForObserver::c_DevilFightingStop,
		NULL);


	NotificationCenter::getInstance()->addObserver(
		this,
		callfuncO_selector(SnagForestLayer::handleBalloonStop),
		MsgTypeForObserver::c_BalloonStop,
		NULL);

	NotificationCenter::getInstance()->addObserver(
		this,
		callfuncO_selector(SnagForestLayer::handleBattleStop),
		MsgTypeForObserver::c_BattleStop,
		NULL);
}

void SnagForestLayer::triggerDevil()
{
	NotificationCenter::getInstance()->postNotification(MsgTypeForObserver::c_DevilFightingStart, NULL);
	pause();
}



/* === Little Game Action ===*/
void SnagForestLayer::handleBalloonStop(Ref* pData)
{
	unsigned int slotPosIdx;
	if (m_littleGameSlot != NULL)
	{
		m_isLittleGameStart = false;
		do
		{
			slotPosIdx = MsgTypeForObserver::getRand(0,m_slotPos->size()-1);
		}while(slotPosIdx == m_slotPosIdx);
		m_slotPosIdx = slotPosIdx;
		m_littleGameSlot->setPosition(m_slotPos->at(m_slotPosIdx));
	}
	recover();
}


bool SnagForestLayer::removeDevil()
{
	if (m_devil != NULL)
	{
		m_devil->release();
		m_devil = NULL;
		return true;
	}
	return false;
}




void SnagForestLayer::pause()
{
	m_listener->setEnabled(false);
	m_ball->getPhysicsBody()->setDynamic(false);
	this->unscheduleUpdate();
}

void SnagForestLayer::recover()
{
	m_ball->getPhysicsBody()->setDynamic(true);
	this->scheduleUpdate();
}
