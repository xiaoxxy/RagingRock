#include "PolySprite.h"

#include "cocos2d.h"

using namespace cocos2d;

PolySprite* PolySprite::create(const char *pFile,
	const Point *uvs,
	int verCnt,
	const int *indices)
{
	PolySprite *pobSprite = new PolySprite();
	//��������
	if (pobSprite &&
		pobSprite->initWithFile(pFile) &&
		pobSprite->initWithUV(uvs, indices, verCnt)) {

		pobSprite->autorelease();
		return pobSprite;
	}
	CC_SAFE_DELETE(pobSprite);

	return NULL;
}

PolySprite* PolySprite::create(Texture2D* texture,
	const Point *uvs,
	int verCnt,
	const int *indices)
{
	PolySprite *pobSprite = new PolySprite();
	//��������
	if (pobSprite &&
		pobSprite->initWithTexture(texture) &&
		pobSprite->initWithUV(uvs, indices, verCnt)) {

		pobSprite->autorelease();
		return pobSprite;
	}
	CC_SAFE_DELETE(pobSprite);
	return NULL;
}

PolySprite::~PolySprite()
{
	releasePoly();
}

//��ʼ��������Ϣ
bool PolySprite::initWithUV(const Point *uvs,
	const int *indices,
	int verCnt)
{
	//�ڴ����
	vertexs_ = new Vertex2F[verCnt];
	uvs_ = new Vertex2F[verCnt];
	indices_ = new unsigned short[(verCnt - 2) * 3];
	colors_ = new unsigned char[verCnt * 4];

	//ʧ�ܴ���
	if (!vertexs_ || !uvs_ || !indices_ || !colors_) {
		releasePoly();
		return false;
	}

	//��ͼ��С
	Size rc = this->getTexture()->getContentSize();

	for (int i = 0; i < verCnt; ++i) {
		//�������������Լ������С���㶥������
		vertexs_[i].x = uvs[i].x*rc.width;
		//cocos2dx�������������Ͻ�Ϊԭ��
		vertexs_[i].y = (1.0 - uvs[i].y)*rc.height;

		uvs_[i].x = uvs[i].x;
		uvs_[i].y = uvs[i].y;
	}

	for (int i = 0; i < (verCnt - 2) * 3; ++i)
		indices_[i] = indices[i];

	memset(colors_, 255, sizeof(unsigned char)*verCnt * 4);

	verCnt_ = verCnt;

	translate(getCenter());

	return true;
}

//�����е�
Point PolySprite::getCenter()
{
	if (!vertexs_) return Point(0, 0);

	float minx = vertexs_[0].x,
		maxx = vertexs_[0].x,
		miny = vertexs_[0].y,
		maxy = vertexs_[0].y;

	//�������ж�����������ĵ�����
	for (int i = 0; i < verCnt_; ++i) {
		minx = minx>vertexs_[i].x ? vertexs_[i].x : minx;
		maxx = maxx;

		miny = miny > vertexs_[i].y ? vertexs_[i].y : miny;
		maxy = maxy;
	}

	return Point((minx + maxx)*0.5, (miny + maxy)*0.5);
}

void PolySprite::translate(const cocos2d::Point& pos)
{
	//����ê��
	//Size rc = m_pobTexture->getContentSize();
	Size rc = this->getTexture()->getContentSize();
	setAnchorPoint(Point(pos.x / rc.width, pos.y / rc.height));
}

void PolySprite::drawPoly(const kmMat4 &transform, bool transformUpdated)
{
	kmGLPushMatrix();
	kmGLLoadMatrix(&transform);

	CHECK_GL_ERROR_DEBUG();

	GL::blendFunc(this->getBlendFunc().src, this->getBlendFunc().dst);

	if (this->getTexture() != NULL) {
		GL::bindTexture2D(this->getTexture()->getName());
	}
	else {
		GL::bindTexture2D(0);
	}

	GL::enableVertexAttribs(GL::VERTEX_ATTRIB_FLAG_POS_COLOR_TEX);

	//���㣬������ɫ
	glVertexAttribPointer(GLProgram::VERTEX_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 0, vertexs_);
	glVertexAttribPointer(GLProgram::VERTEX_ATTRIB_TEX_COORDS, 2, GL_FLOAT, GL_FALSE, 0, uvs_);
	glVertexAttribPointer(GLProgram::VERTEX_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, colors_);
	//��������draw������
	glDrawElements(GL_TRIANGLES, (verCnt_ - 2) * 3, GL_UNSIGNED_SHORT, indices_);

	CHECK_GL_ERROR_DEBUG();
	kmGLPopMatrix();
}

void PolySprite::releasePoly()
{
	CC_SAFE_DELETE_ARRAY(vertexs_);
	CC_SAFE_DELETE_ARRAY(uvs_);
	CC_SAFE_DELETE_ARRAY(indices_);
	CC_SAFE_DELETE_ARRAY(colors_);
}

void PolySprite::draw(Renderer *renderer, const kmMat4& transform, bool transformUpdated)
{
	m_customCommand.init(0);
	m_customCommand.func = CC_CALLBACK_0(PolySprite::drawPoly, this, transform, transformUpdated);
	renderer->addCommand(&m_customCommand);
}