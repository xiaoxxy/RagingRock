#ifndef __REVERSIBLE_CARD_H__
#define __REVERSIBLE_CARD_H__

#include "cocos2d.h"
#include "cocos-ext.h"

USING_NS_CC;

#define kInAngleZ (270) //���濨�Ƶ���ʼZ��Ƕ�
#define kInDeltaZ (90) //���濨����ת��Z��ǶȲ�

#define kOutAngleZ (0) //���濨�Ƶ���ʼZ��Ƕ�
#define kOutDeltaZ (90) //���濨����ת��Z��ǶȲ�

class ReversibleCard: public cocos2d::Sprite
{
	enum {
		tag_inCard = 1,
		tag_outCard
	};

public:
	ReversibleCard();
	virtual ~ReversibleCard();

	static ReversibleCard* create(const char* inCardImageName, const char* outCardImageName, float duration);
	virtual bool init(const char* inCardImageName, const char* outCardImageName, float duration);

	void openCard();
	void openCard(float delay, bool isblink = false);

	void verticalTilt(float duration, float deltaY);
	void verticalTilt(float duration, float deltaY, CallFuncN* callback);
	void verticalTilt(float duration, float deltaY, float delay, CallFuncN* callback);
	void verticalTilt(float duration, float deltaY, FiniteTimeAction *extActionBegin, FiniteTimeAction *extActionAfter);

	void setReversibleCardSize(const Size& size);
	const Size& getReversibleCardSize();

	void setCardType(unsigned int type);
	unsigned int getCardType();

	bool isOpened();

private:
	void initData(const char* inCardImageName, const char* outCardImageName, float duration);
	void openCardFinished();

private:
	bool m_isOpening;
	bool m_isOpened;
	float m_duration;

	Size m_reversibleCardSize;

	unsigned int m_cardType;
};

#endif // __REVERSIBLE_CARD_H__