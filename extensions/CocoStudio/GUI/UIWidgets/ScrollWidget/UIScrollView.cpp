/****************************************************************************
 Copyright (c) 2013 cocos2d-x.org
 
 http://www.cocos2d-x.org
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#include "UIScrollView.h"

NS_CC_BEGIN

namespace ui {
    
ScrollInnerContainer::ScrollInnerContainer()
{
    
}
    
ScrollInnerContainer::~ScrollInnerContainer()
{
    
}
    
ScrollInnerContainer* ScrollInnerContainer::create()
{
    ScrollInnerContainer* widget = new ScrollInnerContainer();
    if (widget && widget->init())
    {
        widget->autorelease();
        return widget;
    }
    CC_SAFE_DELETE(widget);
    return NULL;
}
    
const CCSize& ScrollInnerContainer::getLayoutSize()
{
    Widget* parent = getWidgetParent();
    if (parent)
    {
        return parent->getSize();
    }
    else
    {
        return _size;
    }
}

static const float AUTOSCROLLMAXSPEED = 1000.0f;

const CCPoint SCROLLDIR_UP = CCPoint(0.0f, 1.0f);
const CCPoint SCROLLDIR_DOWN = CCPoint(0.0f, -1.0f);
const CCPoint SCROLLDIR_LEFT = CCPoint(-1.0f, 0.0f);
const CCPoint SCROLLDIR_RIGHT = CCPoint(1.0f, 0.0f);

ScrollView::ScrollView():
_innerContainer(NULL),
_direction(SCROLLVIEW_DIR_VERTICAL),
_touchBeganPoint(CCPointZero),
_touchMovedPoint(CCPointZero),
_touchEndedPoint(CCPointZero),
_touchMovingPoint(CCPointZero),
_autoScrollDir(CCPointZero),
_topBoundary(0.0f),
_bottomBoundary(0.0f),
_leftBoundary(0.0f),
_rightBoundary(0.0f),
_bounceTopBoundary(0.0f),
_bounceBottomBoundary(0.0f),
_bounceLeftBoundary(0.0f),
_bounceRightBoundary(0.0f),
_autoScroll(false),
_autoScrollAddUpTime(0.0f),
_autoScrollOriginalSpeed(0.0f),
_autoScrollAcceleration(-1000.0f),
_isAutoScrollSpeedAttenuated(false),
_needCheckAutoScrollDestination(false),
_autoScrollDestination(CCPointZero),
_bePressed(false),
_slidTime(0.0f),
_moveChildPoint(CCPointZero),
_childFocusCancelOffset(5.0f),
_leftBounceNeeded(false),
_topBounceNeeded(false),
_rightBounceNeeded(false),
_bottomBounceNeeded(false),
_bounceEnabled(false),
_bouncing(false),
_bounceDir(CCPointZero),
_bounceOriginalSpeed(0.0f),
_inertiaScrollEnabled(true),
_scrollViewEventListener(NULL),
_scrollViewEventSelector(NULL)
{
}

ScrollView::~ScrollView()
{
    _scrollViewEventListener = NULL;
    _scrollViewEventSelector = NULL;
}

ScrollView* ScrollView::create()
{
    ScrollView* widget = new ScrollView();
    if (widget && widget->init())
    {
        widget->autorelease();
        return widget;
    }
    CC_SAFE_DELETE(widget);
    return NULL;
}
    
void ScrollView::onEnter()
{
    Layout::onEnter();
    scheduleUpdate();
}

bool ScrollView::init()
{
    if (Layout::init())
    {
        setTouchEnabled(true);
        setClippingEnabled(true);
        _innerContainer->setTouchEnabled(false);
        return true;
    }
    return false;
}

void ScrollView::initRenderer()
{
    Layout::initRenderer();
    _innerContainer = ScrollInnerContainer::create();
    Layout::addChild(_innerContainer,1,1);
}

void ScrollView::onSizeChanged()
{
    Layout::onSizeChanged();
    _topBoundary = _size.height;
    _rightBoundary = _size.width;
    float bounceBoundaryParameterX = _size.width / 3.0f;
    float bounceBoundaryParameterY = _size.height / 3.0f;
    _bounceTopBoundary = _size.height - bounceBoundaryParameterY;
    _bounceBottomBoundary = bounceBoundaryParameterY;
    _bounceLeftBoundary = bounceBoundaryParameterX;
    _bounceRightBoundary = _size.width - bounceBoundaryParameterX;
    CCSize innerSize = _innerContainer->getSize();
    float orginInnerSizeWidth = innerSize.width;
    float orginInnerSizeHeight = innerSize.height;
    float innerSizeWidth = MAX(orginInnerSizeWidth, _size.width);
    float innerSizeHeight = MAX(orginInnerSizeHeight, _size.height);
    _innerContainer->setSize(CCSize(innerSizeWidth, innerSizeHeight));
    _innerContainer->setPosition(CCPoint(0, _size.height - _innerContainer->getSize().height));
}

void ScrollView::setInnerContainerSize(const CCSize &size)
{
    float innerSizeWidth = _size.width;
    float innerSizeHeight = _size.height;
    CCSize originalInnerSize = _innerContainer->getSize();
    if (size.width < _size.width)
    {
        CCLOG("Inner width <= scrollview width, it will be force sized!");
    }
    else
    {
        innerSizeWidth = size.width;
    }
    if (size.height < _size.height)
    {
        CCLOG("Inner height <= scrollview height, it will be force sized!");
    }
    else
    {
        innerSizeHeight = size.height;
    }
    _innerContainer->setSize(CCSize(innerSizeWidth, innerSizeHeight));

    switch (_direction)
    {
        case SCROLLVIEW_DIR_VERTICAL:
        {
            CCSize newInnerSize = _innerContainer->getSize();
            float offset = originalInnerSize.height - newInnerSize.height;
            scrollChildren(0.0f, offset);
            break;
        }
        case SCROLLVIEW_DIR_HORIZONTAL:
        {
            if (_innerContainer->getRightInParent() <= _size.width)
            {
                CCSize newInnerSize = _innerContainer->getSize();
                float offset = originalInnerSize.width - newInnerSize.width;
                scrollChildren(offset, 0.0f);
            }
            break;
        }
        case SCROLLVIEW_DIR_BOTH:
        {
            CCSize newInnerSize = _innerContainer->getSize();
            float offsetY = originalInnerSize.height - newInnerSize.height;
            float offsetX = 0.0f;
            if (_innerContainer->getRightInParent() <= _size.width)
            {
                offsetX = originalInnerSize.width - newInnerSize.width;
            }
            scrollChildren(offsetX, offsetY);
            break;
        }
        default:
            break;
    }
    if (_innerContainer->getLeftInParent() > 0.0f)
    {
        _innerContainer->setPosition(CCPoint(_innerContainer->getAnchorPoint().x * _innerContainer->getSize().width, _innerContainer->getPosition().y));
    }
    if (_innerContainer->getRightInParent() < _size.width)
    {
         _innerContainer->setPosition(CCPoint(_size.width - ((1.0f - _innerContainer->getAnchorPoint().x) * _innerContainer->getSize().width), _innerContainer->getPosition().y));
    }
    if (_innerContainer->getPosition().y > 0.0f)
    {
        _innerContainer->setPosition(CCPoint(_innerContainer->getPosition().x, _innerContainer->getAnchorPoint().y * _innerContainer->getSize().height));
    }
    if (_innerContainer->getTopInParent() < _size.height)
    {
        _innerContainer->setPosition(CCPoint(_innerContainer->getPosition().x, _size.height - (1.0f - _innerContainer->getAnchorPoint().y) * _innerContainer->getSize().height));
    }
}

const CCSize& ScrollView::getInnerContainerSize() const
{
	return _innerContainer->getSize();
}
    
void ScrollView::addChild(CCNode *child)
{
    addChild(child, child->getZOrder(), child->getTag());
}

void ScrollView::addChild(CCNode * child, int zOrder)
{
    addChild(child, zOrder, child->getTag());
}

void ScrollView::addChild(CCNode *child, int zOrder, int tag)
{
    return _innerContainer->addChild(child, zOrder, tag);
}
    
void ScrollView::removeChild(CCNode *child)
{
    removeChild(child, true);
}
    
void ScrollView::removeChild(CCNode* child, bool cleanup)
{
    _innerContainer->removeChild(child, cleanup);
}
    
void ScrollView::removeAllChildren()
{
    removeAllChildrenWithCleanup(true);
}

void ScrollView::removeAllChildrenWithCleanup(bool cleanup)
{
    _innerContainer->removeAllChildrenWithCleanup(cleanup);
}

CCArray* ScrollView::getChildren()
{
    return _innerContainer->getChildren();
}

unsigned int ScrollView::getChildrenCount() const
{
    return _innerContainer->getChildrenCount();
}
    
CCNode* ScrollView::getChildByTag(int tag)
{
    return _innerContainer->getChildByTag(tag);
}
    
Widget* ScrollView::getChildByName(const char *name)
{
    return _innerContainer->getChildByName(name);
}
    
void ScrollView::addNode(CCNode* node)
{
    Layout::addNode(node);
}

void ScrollView::addNode(CCNode * node, int zOrder)
{
    Layout::addNode(node, zOrder);
}

void ScrollView::addNode(CCNode* node, int zOrder, int tag)
{
    _innerContainer->addNode(node, zOrder, tag);
}

CCNode* ScrollView::getNodeByTag(int tag)
{
    return _innerContainer->getNodeByTag(tag);
}
    
void ScrollView::removeNodeByTag(int tag)
{
    _innerContainer->removeNodeByTag(tag);
}

CCArray* ScrollView::getNodes()
{
    return _innerContainer->getNodes();
}

void ScrollView::removeNode(CCNode* node)
{
    _innerContainer->removeNode(node);
}

void ScrollView::removeAllNodes()
{
    _innerContainer->removeAllNodes();
}

void ScrollView::moveChildren(float offsetX, float offsetY)
{
    _moveChildPoint = _innerContainer->getPosition() + CCPoint(offsetX, offsetY);
    _innerContainer->setPosition(_moveChildPoint);
}

void ScrollView::autoScrollChildren(float dt)
{
    float lastTime = _autoScrollAddUpTime;
    _autoScrollAddUpTime += dt;
    if (_isAutoScrollSpeedAttenuated)
    {
        float nowSpeed = _autoScrollOriginalSpeed + _autoScrollAcceleration * _autoScrollAddUpTime;
        if (nowSpeed <= 0.0f)
        {
            stopAutoScrollChildren();
            checkNeedBounce();
        }
        else
        {
            float timeParam = lastTime * 2 + dt;
            float offset = (_autoScrollOriginalSpeed + _autoScrollAcceleration * timeParam * 0.5f) * dt;
            float offsetX = offset * _autoScrollDir.x;
            float offsetY = offset * _autoScrollDir.y;
            if (!scrollChildren(offsetX, offsetY))
            {
                stopAutoScrollChildren();
                checkNeedBounce();
            }
        }
    }
    else
    {
        if (_needCheckAutoScrollDestination)
        {
            float xOffset = _autoScrollDir.x * dt * _autoScrollOriginalSpeed;
            float yOffset = _autoScrollDir.y * dt * _autoScrollOriginalSpeed;
            bool notDone = checkCustomScrollDestination(&xOffset, &yOffset);
            bool scrollCheck = scrollChildren(xOffset, yOffset);
            if (!notDone || !scrollCheck)
            {
                stopAutoScrollChildren();
                checkNeedBounce();
            }
        }
        else
        {
            if (!scrollChildren(_autoScrollDir.x * dt * _autoScrollOriginalSpeed, _autoScrollDir.y * dt * _autoScrollOriginalSpeed))
            {
                stopAutoScrollChildren();
                checkNeedBounce();
            }
        }
    }
}

void ScrollView::bounceChildren(float dt)
{
    if (_bounceOriginalSpeed <= 0.0f)
    {
        stopBounceChildren();
    }
    if (!bounceScrollChildren(_bounceDir.x * dt * _bounceOriginalSpeed, _bounceDir.y * dt * _bounceOriginalSpeed))
    {
        stopBounceChildren();
    }
}

bool ScrollView::checkNeedBounce()
{
    if (!_bounceEnabled)
    {
        return false;
    }
    checkBounceBoundary();
    if (_topBounceNeeded || _bottomBounceNeeded || _leftBounceNeeded || _rightBounceNeeded)
    {
        if (_topBounceNeeded && _leftBounceNeeded)
        {
            CCPoint scrollVector = CCPoint(0.0f, _size.height) - CCPoint(_innerContainer->getLeftInParent(), _innerContainer->getTopInParent());
            float orSpeed = scrollVector.getLength()/(0.2f);
            _bounceDir = scrollVector.getNormalized();
            startBounceChildren(orSpeed);
        }
        else if (_topBounceNeeded && _rightBounceNeeded)
        {
            CCPoint scrollVector = CCPoint(_size.width, _size.height) - CCPoint(_innerContainer->getRightInParent(), _innerContainer->getTopInParent());
            float orSpeed = scrollVector.getLength()/(0.2f);
            _bounceDir = scrollVector.getNormalized();
            startBounceChildren(orSpeed);
        }
        else if (_bottomBounceNeeded && _leftBounceNeeded)
        {
            CCPoint scrollVector = CCPointZero - CCPoint(_innerContainer->getLeftInParent(), _innerContainer->getBottomInParent());
            float orSpeed = scrollVector.getLength()/(0.2f);
            _bounceDir = scrollVector.getNormalized();
            startBounceChildren(orSpeed);
        }
        else if (_bottomBounceNeeded && _rightBounceNeeded)
        {
            CCPoint scrollVector = CCPoint(_size.width, 0.0f) - CCPoint(_innerContainer->getRightInParent(), _innerContainer->getBottomInParent());
            float orSpeed = scrollVector.getLength()/(0.2f);
            _bounceDir = scrollVector.getNormalized();
            startBounceChildren(orSpeed);
        }
        else if (_topBounceNeeded)
        {
            CCPoint scrollVector = CCPoint(0.0f, _size.height) - CCPoint(0.0f, _innerContainer->getTopInParent());
            float orSpeed = scrollVector.getLength()/(0.2f);
            _bounceDir = scrollVector.getNormalized();
            startBounceChildren(orSpeed);
        }
        else if (_bottomBounceNeeded)
        {
            CCPoint scrollVector = CCPointZero - CCPoint(0.0f, _innerContainer->getBottomInParent());
            float orSpeed = scrollVector.getLength()/(0.2f);
            _bounceDir = scrollVector.getNormalized();
            startBounceChildren(orSpeed);
        }
        else if (_leftBounceNeeded)
        {
            CCPoint scrollVector = CCPointZero - CCPoint(_innerContainer->getLeftInParent(), 0.0f);
            float orSpeed = scrollVector.getLength()/(0.2f);
            _bounceDir = scrollVector.getNormalized();
            startBounceChildren(orSpeed);
        }
        else if (_rightBounceNeeded)
        {
            CCPoint scrollVector = CCPoint(_size.width, 0.0f) - CCPoint(_innerContainer->getRightInParent(), 0.0f);
            float orSpeed = scrollVector.getLength()/(0.2f);
            _bounceDir = scrollVector.getNormalized();
            startBounceChildren(orSpeed);
        }
        return true;
    }
    return false;
}

void ScrollView::checkBounceBoundary()
{
    float icBottomPos = _innerContainer->getBottomInParent();
    if (icBottomPos > _bottomBoundary)
    {
        scrollToBottomEvent();
        _bottomBounceNeeded = true;
    }
    else
    {
        _bottomBounceNeeded = false;
    }
    float icTopPos = _innerContainer->getTopInParent();
    if (icTopPos < _topBoundary)
    {
        scrollToTopEvent();
        _topBounceNeeded = true;
    }
    else
    {
        _topBounceNeeded = false;
    }
    float icRightPos = _innerContainer->getRightInParent();
    if (icRightPos < _rightBoundary)
    {
        scrollToRightEvent();
        _rightBounceNeeded = true;
    }
    else
    {
        _rightBounceNeeded = false;
    }
    float icLeftPos = _innerContainer->getLeftInParent();
    if (icLeftPos > _leftBoundary)
    {
        scrollToLeftEvent();
        _leftBounceNeeded = true;
    }
    else
    {
        _leftBounceNeeded = false;
    }
}

void ScrollView::startBounceChildren(float v)
{
    _bounceOriginalSpeed = v;
    _bouncing = true;
}

void ScrollView::stopBounceChildren()
{
    _bouncing = false;
    _bounceOriginalSpeed = 0.0f;
    _leftBounceNeeded = false;
    _rightBounceNeeded = false;
    _topBounceNeeded = false;
    _bottomBounceNeeded = false;
}

void ScrollView::startAutoScrollChildrenWithOriginalSpeed(const CCPoint& dir, float v, bool attenuated, float acceleration)
{
    stopAutoScrollChildren();
    _autoScrollDir = dir;
    _isAutoScrollSpeedAttenuated = attenuated;
    _autoScrollOriginalSpeed = v;
    _autoScroll = true;
    _autoScrollAcceleration = acceleration;
}

void ScrollView::startAutoScrollChildrenWithDestination(const CCPoint& des, float time, bool attenuated)
{
    _needCheckAutoScrollDestination = false;
    _autoScrollDestination = des;
    CCPoint dis = des - _innerContainer->getPosition();
    CCPoint dir = dis.getNormalized();
    float orSpeed = 0.0f;
    float acceleration = -1000.0f;
    if (attenuated)
    {
        acceleration = (-(2 * dis.getLength())) / (time * time);
        orSpeed =  2 * dis.getLength() / time;
    }
    else
    {
        _needCheckAutoScrollDestination = true;
        orSpeed = dis.getLength() / time;
    }
    startAutoScrollChildrenWithOriginalSpeed(dir, orSpeed, attenuated, acceleration);
}

void ScrollView::jumpToDestination(const CCPoint &des)
{
    float finalOffsetX = des.x;
    float finalOffsetY = des.y;
    switch (_direction)
    {
        case SCROLLVIEW_DIR_VERTICAL:
            if (des.y <= 0)
            {
                finalOffsetY = MAX(des.y, _size.height - _innerContainer->getSize().height);
            }
            break;
        case SCROLLVIEW_DIR_HORIZONTAL:
            if (des.x <= 0)
            {
                finalOffsetX = MAX(des.x, _size.width - _innerContainer->getSize().width);
            }
            break;
        case SCROLLVIEW_DIR_BOTH:
            if (des.y <= 0)
            {
                finalOffsetY = MAX(des.y, _size.height - _innerContainer->getSize().height);
            }
            if (des.x <= 0)
            {
                finalOffsetX = MAX(des.x, _size.width - _innerContainer->getSize().width);
            }
            break;
        default:
            break;
    }
    _innerContainer->setPosition(CCPoint(finalOffsetX, finalOffsetY));
}

void ScrollView::stopAutoScrollChildren()
{
    _autoScroll = false;
    _autoScrollOriginalSpeed = 0.0f;
    _autoScrollAddUpTime = 0.0f;
}

bool ScrollView::bounceScrollChildren(float touchOffsetX, float touchOffsetY)
{
    bool scrollenabled = true;
    if (touchOffsetX > 0.0f && touchOffsetY > 0.0f) //first quadrant //bounce to top-right
    {
        float realOffsetX = touchOffsetX;
        float realOffsetY = touchOffsetY;
        float icRightPos = _innerContainer->getRightInParent();
        if (icRightPos + realOffsetX >= _rightBoundary)
        {
            realOffsetX = _rightBoundary - icRightPos;
            bounceRightEvent();
            scrollenabled = false;
        }
        float icTopPos = _innerContainer->getTopInParent();
        if (icTopPos + touchOffsetY >= _topBoundary)
        {
            realOffsetY = _topBoundary - icTopPos;
            bounceTopEvent();
            scrollenabled = false;
        }
        moveChildren(realOffsetX, realOffsetY);
    }
    else if(touchOffsetX < 0.0f && touchOffsetY > 0.0f) //second quadrant //bounce to top-left
    {
        float realOffsetX = touchOffsetX;
        float realOffsetY = touchOffsetY;
        float icLefrPos = _innerContainer->getLeftInParent();
        if (icLefrPos + realOffsetX <= _leftBoundary)
        {
            realOffsetX = _leftBoundary - icLefrPos;
            bounceLeftEvent();
            scrollenabled = false;
        }
        float icTopPos = _innerContainer->getTopInParent();
        if (icTopPos + touchOffsetY >= _topBoundary)
        {
            realOffsetY = _topBoundary - icTopPos;
            bounceTopEvent();
            scrollenabled = false;
        }
        moveChildren(realOffsetX, realOffsetY);
    }
    else if (touchOffsetX < 0.0f && touchOffsetY < 0.0f) //third quadrant //bounce to bottom-left
    {
        float realOffsetX = touchOffsetX;
        float realOffsetY = touchOffsetY;
        float icLefrPos = _innerContainer->getLeftInParent();
        if (icLefrPos + realOffsetX <= _leftBoundary)
        {
            realOffsetX = _leftBoundary - icLefrPos;
            bounceLeftEvent();
            scrollenabled = false;
        }
        float icBottomPos = _innerContainer->getBottomInParent();
        if (icBottomPos + touchOffsetY <= _bottomBoundary)
        {
            realOffsetY = _bottomBoundary - icBottomPos;
            bounceBottomEvent();
            scrollenabled = false;
        }
        moveChildren(realOffsetX, realOffsetY);
    }
    else if (touchOffsetX > 0.0f && touchOffsetY < 0.0f) //forth quadrant //bounce to bottom-right
    {
        float realOffsetX = touchOffsetX;
        float realOffsetY = touchOffsetY;
        float icRightPos = _innerContainer->getRightInParent();
        if (icRightPos + realOffsetX >= _rightBoundary)
        {
            realOffsetX = _rightBoundary - icRightPos;
            bounceRightEvent();
            scrollenabled = false;
        }
        float icBottomPos = _innerContainer->getBottomInParent();
        if (icBottomPos + touchOffsetY <= _bottomBoundary)
        {
            realOffsetY = _bottomBoundary - icBottomPos;
            bounceBottomEvent();
            scrollenabled = false;
        }
        moveChildren(realOffsetX, realOffsetY);
    }
    else if (touchOffsetX == 0.0f && touchOffsetY > 0.0f) // bounce to top
    {
        float realOffsetY = touchOffsetY;
        float icTopPos = _innerContainer->getTopInParent();
        if (icTopPos + touchOffsetY >= _topBoundary)
        {
            realOffsetY = _topBoundary - icTopPos;
            bounceTopEvent();
            scrollenabled = false;
        }
        moveChildren(0.0f, realOffsetY);
    }
    else if (touchOffsetX == 0.0f && touchOffsetY < 0.0f) //bounce to bottom
    {
        float realOffsetY = touchOffsetY;
        float icBottomPos = _innerContainer->getBottomInParent();
        if (icBottomPos + touchOffsetY <= _bottomBoundary)
        {
            realOffsetY = _bottomBoundary - icBottomPos;
            bounceBottomEvent();
            scrollenabled = false;
        }
        moveChildren(0.0f, realOffsetY);
    }
    else if (touchOffsetX > 0.0f && touchOffsetY == 0.0f) //bounce to right
    {
        float realOffsetX = touchOffsetX;
        float icRightPos = _innerContainer->getRightInParent();
        if (icRightPos + realOffsetX >= _rightBoundary)
        {
            realOffsetX = _rightBoundary - icRightPos;
            bounceRightEvent();
            scrollenabled = false;
        }
        moveChildren(realOffsetX, 0.0f);
    }
    else if (touchOffsetX < 0.0f && touchOffsetY == 0.0f) //bounce to left
    {
        float realOffsetX = touchOffsetX;
        float icLeftPos = _innerContainer->getLeftInParent();
        if (icLeftPos + realOffsetX <= _leftBoundary)
        {
            realOffsetX = _leftBoundary - icLeftPos;
            bounceLeftEvent();
            scrollenabled = false;
        }
        moveChildren(realOffsetX, 0.0f);
    }
    return scrollenabled;
}

bool ScrollView::checkCustomScrollDestination(float* touchOffsetX, float* touchOffsetY)
{
    bool scrollenabled = true;
    switch (_direction)
    {
        case SCROLLVIEW_DIR_VERTICAL: // vertical
        {
            if (_autoScrollDir.y > 0)
            {
                float icBottomPos = _innerContainer->getBottomInParent();
                if (icBottomPos + *touchOffsetY >= _autoScrollDestination.y)
                {
                    *touchOffsetY = _autoScrollDestination.y - icBottomPos;
                    scrollenabled = false;
                }
            }
            else
            {
                float icBottomPos = _innerContainer->getBottomInParent();
                if (icBottomPos + *touchOffsetY <= _autoScrollDestination.y)
                {
                    *touchOffsetY = _autoScrollDestination.y - icBottomPos;
                    scrollenabled = false;
                }
            }
            break;
        }
        case SCROLLVIEW_DIR_HORIZONTAL: // horizontal
        {
            if (_autoScrollDir.x > 0)
            {
                float icLeftPos = _innerContainer->getLeftInParent();
                if (icLeftPos + *touchOffsetX >= _autoScrollDestination.x)
                {
                    *touchOffsetX = _autoScrollDestination.x - icLeftPos;
                    scrollenabled = false;
                }
            }
            else
            {
                float icLeftPos = _innerContainer->getLeftInParent();
                if (icLeftPos + *touchOffsetX <= _autoScrollDestination.x)
                {
                    *touchOffsetX = _autoScrollDestination.x - icLeftPos;
                    scrollenabled = false;
                }
            }
            break;
        }
        case SCROLLVIEW_DIR_BOTH:
        {
            if (*touchOffsetX > 0.0f && *touchOffsetY > 0.0f) // up right
            {
                float icLeftPos = _innerContainer->getLeftInParent();
                if (icLeftPos + *touchOffsetX >= _autoScrollDestination.x)
                {
                    *touchOffsetX = _autoScrollDestination.x - icLeftPos;
                    scrollenabled = false;
                }
                float icBottomPos = _innerContainer->getBottomInParent();
                if (icBottomPos + *touchOffsetY >= _autoScrollDestination.y)
                {
                    *touchOffsetY = _autoScrollDestination.y - icBottomPos;
                    scrollenabled = false;
                }
            }
            else if (*touchOffsetX < 0.0f && *touchOffsetY > 0.0f) // up left
            {
                float icRightPos = _innerContainer->getRightInParent();
                if (icRightPos + *touchOffsetX <= _autoScrollDestination.x)
                {
                    *touchOffsetX = _autoScrollDestination.x - icRightPos;
                    scrollenabled = false;
                }
                float icBottomPos = _innerContainer->getBottomInParent();
                if (icBottomPos + *touchOffsetY >= _autoScrollDestination.y)
                {
                    *touchOffsetY = _autoScrollDestination.y - icBottomPos;
                    scrollenabled = false;
                }
            }
            else if (*touchOffsetX < 0.0f && *touchOffsetY < 0.0f) // down left
            {
                float icRightPos = _innerContainer->getRightInParent();
                if (icRightPos + *touchOffsetX <= _autoScrollDestination.x)
                {
                    *touchOffsetX = _autoScrollDestination.x - icRightPos;
                    scrollenabled = false;
                }
                float icTopPos = _innerContainer->getTopInParent();
                if (icTopPos + *touchOffsetY <= _autoScrollDestination.y)
                {
                    *touchOffsetY = _autoScrollDestination.y - icTopPos;
                    scrollenabled = false;
                }
            }
            else if (*touchOffsetX > 0.0f && *touchOffsetY < 0.0f) // down right
            {
                float icLeftPos = _innerContainer->getLeftInParent();
                if (icLeftPos + *touchOffsetX >= _autoScrollDestination.x)
                {
                    *touchOffsetX = _autoScrollDestination.x - icLeftPos;
                    scrollenabled = false;
                }
                float icTopPos = _innerContainer->getTopInParent();
                if (icTopPos + *touchOffsetY <= _autoScrollDestination.y)
                {
                    *touchOffsetY = _autoScrollDestination.y - icTopPos;
                    scrollenabled = false;
                }
            }
            else if (*touchOffsetX == 0.0f && *touchOffsetY > 0.0f) // up
            {
                float icBottomPos = _innerContainer->getBottomInParent();
                if (icBottomPos + *touchOffsetY >= _autoScrollDestination.y)
                {
                    *touchOffsetY = _autoScrollDestination.y - icBottomPos;
                    scrollenabled = false;
                }
            }
            else if (*touchOffsetX < 0.0f && *touchOffsetY == 0.0f) // left
            {
                float icRightPos = _innerContainer->getRightInParent();
                if (icRightPos + *touchOffsetX <= _autoScrollDestination.x)
                {
                    *touchOffsetX = _autoScrollDestination.x - icRightPos;
                    scrollenabled = false;
                }
            }
            else if (*touchOffsetX == 0.0f && *touchOffsetY < 0.0f) // down
            {
                float icTopPos = _innerContainer->getTopInParent();
                if (icTopPos + *touchOffsetY <= _autoScrollDestination.y)
                {
                    *touchOffsetY = _autoScrollDestination.y - icTopPos;
                    scrollenabled = false;
                }
            }
            else if (*touchOffsetX > 0.0f && *touchOffsetY == 0.0f) // right
            {
                float icLeftPos = _innerContainer->getLeftInParent();
                if (icLeftPos + *touchOffsetX >= _autoScrollDestination.x)
                {
                    *touchOffsetX = _autoScrollDestination.x - icLeftPos;
                    scrollenabled = false;
                }
            }
            break;
        }
        default:
            break;
    }
    return scrollenabled;
}

bool ScrollView::scrollChildren(float touchOffsetX, float touchOffsetY)
{
    bool scrollenabled = true;
    scrollingEvent();
    switch (_direction)
    {
        case SCROLLVIEW_DIR_VERTICAL: // vertical
        {
            float realOffset = touchOffsetY;
            if (_bounceEnabled)
            {
                float icBottomPos = _innerContainer->getBottomInParent();
                if (icBottomPos + touchOffsetY >= _bounceBottomBoundary)
                {
                    realOffset = _bounceBottomBoundary - icBottomPos;
                    scrollToBottomEvent();
                    scrollenabled = false;
                }
                float icTopPos = _innerContainer->getTopInParent();
                if (icTopPos + touchOffsetY <= _bounceTopBoundary)
                {
                    realOffset = _bounceTopBoundary - icTopPos;
                    scrollToTopEvent();
                    scrollenabled = false;
                }
            }
            else
            {
                float icBottomPos = _innerContainer->getBottomInParent();
                if (icBottomPos + touchOffsetY >= _bottomBoundary)
                {
                    realOffset = _bottomBoundary - icBottomPos;
                    scrollToBottomEvent();
                    scrollenabled = false;
                }
                float icTopPos = _innerContainer->getTopInParent();
                if (icTopPos + touchOffsetY <= _topBoundary)
                {
                    realOffset = _topBoundary - icTopPos;
                    scrollToTopEvent();
                    scrollenabled = false;
                }
            }
            moveChildren(0.0f, realOffset);
            break;
        }
        case SCROLLVIEW_DIR_HORIZONTAL: // horizontal
        {
            float realOffset = touchOffsetX;
            if (_bounceEnabled)
            {
                float icRightPos = _innerContainer->getRightInParent();
                if (icRightPos + touchOffsetX <= _bounceRightBoundary)
                {
                    realOffset = _bounceRightBoundary - icRightPos;
                    scrollToRightEvent();
                    scrollenabled = false;
                }
                float icLeftPos = _innerContainer->getLeftInParent();
                if (icLeftPos + touchOffsetX >= _bounceLeftBoundary)
                {
                    realOffset = _bounceLeftBoundary - icLeftPos;
                    scrollToLeftEvent();
                    scrollenabled = false;
                }
            }
            else
            {
                float icRightPos = _innerContainer->getRightInParent();
                if (icRightPos + touchOffsetX <= _rightBoundary)
                {
                    realOffset = _rightBoundary - icRightPos;
                    scrollToRightEvent();
                    scrollenabled = false;
                }
                float icLeftPos = _innerContainer->getLeftInParent();
                if (icLeftPos + touchOffsetX >= _leftBoundary)
                {
                    realOffset = _leftBoundary - icLeftPos;
                    scrollToLeftEvent();
                    scrollenabled = false;
                }
            }
            moveChildren(realOffset, 0.0f);
            break;
        }
        case SCROLLVIEW_DIR_BOTH:
        {
            float realOffsetX = touchOffsetX;
            float realOffsetY = touchOffsetY;
            if (_bounceEnabled)
            {
                if (touchOffsetX > 0.0f && touchOffsetY > 0.0f) // up right
                {
                    float icLeftPos = _innerContainer->getLeftInParent();
                    if (icLeftPos + touchOffsetX >= _bounceLeftBoundary)
                    {
                        realOffsetX = _bounceLeftBoundary - icLeftPos;
                        scrollToLeftEvent();
                        scrollenabled = false;
                    }
                    float icBottomPos = _innerContainer->getBottomInParent();
                    if (icBottomPos + touchOffsetY >= _bounceBottomBoundary)
                    {
                        realOffsetY = _bounceBottomBoundary - icBottomPos;
                        scrollToBottomEvent();
                        scrollenabled = false;
                    }
                }
                else if (touchOffsetX < 0.0f && touchOffsetY > 0.0f) // up left
                {
                    float icRightPos = _innerContainer->getRightInParent();
                    if (icRightPos + touchOffsetX <= _bounceRightBoundary)
                    {
                        realOffsetX = _bounceRightBoundary - icRightPos;
                        scrollToRightEvent();
                        scrollenabled = false;
                    }
                    float icBottomPos = _innerContainer->getBottomInParent();
                    if (icBottomPos + touchOffsetY >= _bounceBottomBoundary)
                    {
                        realOffsetY = _bounceBottomBoundary - icBottomPos;
                        scrollToBottomEvent();
                        scrollenabled = false;
                    }
                }
                else if (touchOffsetX < 0.0f && touchOffsetY < 0.0f) // down left
                {
                    float icRightPos = _innerContainer->getRightInParent();
                    if (icRightPos + touchOffsetX <= _bounceRightBoundary)
                    {
                        realOffsetX = _bounceRightBoundary - icRightPos;
                        scrollToRightEvent();
                        scrollenabled = false;
                    }
                    float icTopPos = _innerContainer->getTopInParent();
                    if (icTopPos + touchOffsetY <= _bounceTopBoundary)
                    {
                        realOffsetY = _bounceTopBoundary - icTopPos;
                        scrollToTopEvent();
                        scrollenabled = false;
                    }
                }
                else if (touchOffsetX > 0.0f && touchOffsetY < 0.0f) // down right
                {
                    float icLeftPos = _innerContainer->getLeftInParent();
                    if (icLeftPos + touchOffsetX >= _bounceLeftBoundary)
                    {
                        realOffsetX = _bounceLeftBoundary - icLeftPos;
                        scrollToLeftEvent();
                        scrollenabled = false;
                    }
                    float icTopPos = _innerContainer->getTopInParent();
                    if (icTopPos + touchOffsetY <= _bounceTopBoundary)
                    {
                        realOffsetY = _bounceTopBoundary - icTopPos;
                        scrollToTopEvent();
                        scrollenabled = false;
                    }
                }
                else if (touchOffsetX == 0.0f && touchOffsetY > 0.0f) // up
                {
                    float icBottomPos = _innerContainer->getBottomInParent();
                    if (icBottomPos + touchOffsetY >= _bounceBottomBoundary)
                    {
                        realOffsetY = _bounceBottomBoundary - icBottomPos;
                        scrollToBottomEvent();
                        scrollenabled = false;
                    }
                }
                else if (touchOffsetX < 0.0f && touchOffsetY == 0.0f) // left
                {
                    float icRightPos = _innerContainer->getRightInParent();
                    if (icRightPos + touchOffsetX <= _bounceRightBoundary)
                    {
                        realOffsetX = _bounceRightBoundary - icRightPos;
                        scrollToRightEvent();
                        scrollenabled = false;
                    }
                }
                else if (touchOffsetX == 0.0f && touchOffsetY < 0.0f) // down
                {
                    float icTopPos = _innerContainer->getTopInParent();
                    if (icTopPos + touchOffsetY <= _bounceTopBoundary)
                    {
                        realOffsetY = _bounceTopBoundary - icTopPos;
                        scrollToTopEvent();
                        scrollenabled = false;
                    }
                }
                else if (touchOffsetX > 0.0f && touchOffsetY == 0.0f) // right
                {
                    float icLeftPos = _innerContainer->getLeftInParent();
                    if (icLeftPos + touchOffsetX >= _bounceLeftBoundary)
                    {
                        realOffsetX = _bounceLeftBoundary - icLeftPos;
                        scrollToLeftEvent();
                        scrollenabled = false;
                    }
                }
            }
            else
            {
                if (touchOffsetX > 0.0f && touchOffsetY > 0.0f) // up right
                {
                    float icLeftPos = _innerContainer->getLeftInParent();
                    if (icLeftPos + touchOffsetX >= _leftBoundary)
                    {
                        realOffsetX = _leftBoundary - icLeftPos;
                        scrollToLeftEvent();
                        scrollenabled = false;
                    }
                    float icBottomPos = _innerContainer->getBottomInParent();
                    if (icBottomPos + touchOffsetY >= _bottomBoundary)
                    {
                        realOffsetY = _bottomBoundary - icBottomPos;
                        scrollToBottomEvent();
                        scrollenabled = false;
                    }
                }
                else if (touchOffsetX < 0.0f && touchOffsetY > 0.0f) // up left
                {
                    float icRightPos = _innerContainer->getRightInParent();
                    if (icRightPos + touchOffsetX <= _rightBoundary)
                    {
                        realOffsetX = _rightBoundary - icRightPos;
                        scrollToRightEvent();
                        scrollenabled = false;
                    }
                    float icBottomPos = _innerContainer->getBottomInParent();
                    if (icBottomPos + touchOffsetY >= _bottomBoundary)
                    {
                        realOffsetY = _bottomBoundary - icBottomPos;
                        scrollToBottomEvent();
                        scrollenabled = false;
                    }
                }
                else if (touchOffsetX < 0.0f && touchOffsetY < 0.0f) // down left
                {
                    float icRightPos = _innerContainer->getRightInParent();
                    if (icRightPos + touchOffsetX <= _rightBoundary)
                    {
                        realOffsetX = _rightBoundary - icRightPos;
                        scrollToRightEvent();
                        scrollenabled = false;
                    }
                    float icTopPos = _innerContainer->getTopInParent();
                    if (icTopPos + touchOffsetY <= _topBoundary)
                    {
                        realOffsetY = _topBoundary - icTopPos;
                        scrollToTopEvent();
                        scrollenabled = false;
                    }
                }
                else if (touchOffsetX > 0.0f && touchOffsetY < 0.0f) // down right
                {
                    float icLeftPos = _innerContainer->getLeftInParent();
                    if (icLeftPos + touchOffsetX >= _leftBoundary)
                    {
                        realOffsetX = _leftBoundary - icLeftPos;
                        scrollToLeftEvent();
                        scrollenabled = false;
                    }
                    float icTopPos = _innerContainer->getTopInParent();
                    if (icTopPos + touchOffsetY <= _topBoundary)
                    {
                        realOffsetY = _topBoundary - icTopPos;
                        scrollToTopEvent();
                        scrollenabled = false;
                    }
                }
                else if (touchOffsetX == 0.0f && touchOffsetY > 0.0f) // up
                {
                    float icBottomPos = _innerContainer->getBottomInParent();
                    if (icBottomPos + touchOffsetY >= _bottomBoundary)
                    {
                        realOffsetY = _bottomBoundary - icBottomPos;
                        scrollToBottomEvent();
                        scrollenabled = false;
                    }
                }
                else if (touchOffsetX < 0.0f && touchOffsetY == 0.0f) // left
                {
                    float icRightPos = _innerContainer->getRightInParent();
                    if (icRightPos + touchOffsetX <= _rightBoundary)
                    {
                        realOffsetX = _rightBoundary - icRightPos;
                        scrollToRightEvent();
                        scrollenabled = false;
                    }
                }
                else if (touchOffsetX == 0.0f && touchOffsetY < 0.0f) // down
                {
                    float icTopPos = _innerContainer->getTopInParent();
                    if (icTopPos + touchOffsetY <= _topBoundary)
                    {
                        realOffsetY = _topBoundary - icTopPos;
                        scrollToTopEvent();
                        scrollenabled = false;
                    }
                }
                else if (touchOffsetX > 0.0f && touchOffsetY == 0.0f) // right
                {
                    float icLeftPos = _innerContainer->getLeftInParent();
                    if (icLeftPos + touchOffsetX >= _leftBoundary)
                    {
                        realOffsetX = _leftBoundary - icLeftPos;
                        scrollToLeftEvent();
                        scrollenabled = false;
                    }
                }
            }
            moveChildren(realOffsetX, realOffsetY);
            break;
        }
        default:
            break;
    }
    return scrollenabled;
}

void ScrollView::scrollToBottom(float time, bool attenuated)
{
    startAutoScrollChildrenWithDestination(CCPoint(_innerContainer->getPosition().x, 0.0f), time, attenuated);
}

void ScrollView::scrollToTop(float time, bool attenuated)
{
    startAutoScrollChildrenWithDestination(CCPoint(_innerContainer->getPosition().x, _size.height - _innerContainer->getSize().height), time, attenuated);
}

void ScrollView::scrollToLeft(float time, bool attenuated)
{
    startAutoScrollChildrenWithDestination(CCPoint(0.0f, _innerContainer->getPosition().y), time, attenuated);
}

void ScrollView::scrollToRight(float time, bool attenuated)
{
    startAutoScrollChildrenWithDestination(CCPoint(_size.width - _innerContainer->getSize().width, _innerContainer->getPosition().y), time, attenuated);
}

void ScrollView::scrollToTopLeft(float time, bool attenuated)
{
    if (_direction != SCROLLVIEW_DIR_BOTH)
    {
        CCLOG("Scroll diretion is not both!");
        return;
    }
    startAutoScrollChildrenWithDestination(CCPoint(0.0f, _size.height - _innerContainer->getSize().height), time, attenuated);
}

void ScrollView::scrollToTopRight(float time, bool attenuated)
{
    if (_direction != SCROLLVIEW_DIR_BOTH)
    {
        CCLOG("Scroll diretion is not both!");
        return;
    }
    startAutoScrollChildrenWithDestination(CCPoint(_size.width - _innerContainer->getSize().width, _size.height - _innerContainer->getSize().height), time, attenuated);
}

void ScrollView::scrollToBottomLeft(float time, bool attenuated)
{
    if (_direction != SCROLLVIEW_DIR_BOTH)
    {
        CCLOG("Scroll diretion is not both!");
        return;
    }
    startAutoScrollChildrenWithDestination(CCPointZero, time, attenuated);
}

void ScrollView::scrollToBottomRight(float time, bool attenuated)
{
    if (_direction != SCROLLVIEW_DIR_BOTH)
    {
        CCLOG("Scroll diretion is not both!");
        return;
    }
    startAutoScrollChildrenWithDestination(CCPoint(_size.width - _innerContainer->getSize().width, 0.0f), time, attenuated);
}

void ScrollView::scrollToPercentVertical(float percent, float time, bool attenuated)
{
    float minY = _size.height - _innerContainer->getSize().height;
    float h = - minY;
    startAutoScrollChildrenWithDestination(CCPoint(_innerContainer->getPosition().x, minY + percent * h / 100.0f), time, attenuated);
}

void ScrollView::scrollToPercentHorizontal(float percent, float time, bool attenuated)
{
    float w = _innerContainer->getSize().width - _size.width;
    startAutoScrollChildrenWithDestination(CCPoint(-(percent * w / 100.0f), _innerContainer->getPosition().y), time, attenuated);
}

void ScrollView::scrollToPercentBothDirection(const CCPoint& percent, float time, bool attenuated)
{
    if (_direction != SCROLLVIEW_DIR_BOTH)
    {
        return;
    }
    float minY = _size.height - _innerContainer->getSize().height;
    float h = - minY;
    float w = _innerContainer->getSize().width - _size.width;
    startAutoScrollChildrenWithDestination(CCPoint(-(percent.x * w / 100.0f), minY + percent.y * h / 100.0f), time, attenuated);
}

void ScrollView::jumpToBottom()
{
    jumpToDestination(CCPoint(_innerContainer->getPosition().x, 0.0f));
}

void ScrollView::jumpToTop()
{
    jumpToDestination(CCPoint(_innerContainer->getPosition().x, _size.height - _innerContainer->getSize().height));
}

void ScrollView::jumpToLeft()
{
    jumpToDestination(CCPoint(0.0f, _innerContainer->getPosition().y));
}

void ScrollView::jumpToRight()
{
    jumpToDestination(CCPoint(_size.width - _innerContainer->getSize().width, _innerContainer->getPosition().y));
}

void ScrollView::jumpToTopLeft()
{
    if (_direction != SCROLLVIEW_DIR_BOTH)
    {
        CCLOG("Scroll diretion is not both!");
        return;
    }
    jumpToDestination(CCPoint(0.0f, _size.height - _innerContainer->getSize().height));
}

void ScrollView::jumpToTopRight()
{
    if (_direction != SCROLLVIEW_DIR_BOTH)
    {
        CCLOG("Scroll diretion is not both!");
        return;
    }
    jumpToDestination(CCPoint(_size.width - _innerContainer->getSize().width, _size.height - _innerContainer->getSize().height));
}

void ScrollView::jumpToBottomLeft()
{
    if (_direction != SCROLLVIEW_DIR_BOTH)
    {
        CCLOG("Scroll diretion is not both!");
        return;
    }
    jumpToDestination(CCPointZero);
}

void ScrollView::jumpToBottomRight()
{
    if (_direction != SCROLLVIEW_DIR_BOTH)
    {
        CCLOG("Scroll diretion is not both!");
        return;
    }
    jumpToDestination(CCPoint(_size.width - _innerContainer->getSize().width, 0.0f));
}

void ScrollView::jumpToPercentVertical(float percent)
{
    float minY = _size.height - _innerContainer->getSize().height;
    float h = - minY;
    jumpToDestination(CCPoint(_innerContainer->getPosition().x, minY + percent * h / 100.0f));
}

void ScrollView::jumpToPercentHorizontal(float percent)
{
    float w = _innerContainer->getSize().width - _size.width;
    jumpToDestination(CCPoint(-(percent * w / 100.0f), _innerContainer->getPosition().y));
}

void ScrollView::jumpToPercentBothDirection(const CCPoint& percent)
{
    if (_direction != SCROLLVIEW_DIR_BOTH)
    {
        return;
    }
    float minY = _size.height - _innerContainer->getSize().height;
    float h = - minY;
    float w = _innerContainer->getSize().width - _size.width;
    jumpToDestination(CCPoint(-(percent.x * w / 100.0f), minY + percent.y * h / 100.0f));
}

void ScrollView::startRecordSlidAction()
{
    if (_autoScroll)
    {
        stopAutoScrollChildren();
    }
    if (_bouncing)
    {
        stopBounceChildren();
    }
    _slidTime = 0.0f;
}

void ScrollView::endRecordSlidAction()
{
    if (!checkNeedBounce() && _inertiaScrollEnabled)
    {
        if (_slidTime <= 0.016f)
        {
            return;
        }
        float totalDis = 0.0f;
        CCPoint dir;
        switch (_direction)
        {
            case SCROLLVIEW_DIR_VERTICAL:
                totalDis = _touchEndedPoint.y - _touchBeganPoint.y;
                if (totalDis < 0.0f)
                {
                    dir = SCROLLDIR_DOWN;
                }
                else
                {
                    dir = SCROLLDIR_UP;
                }
                break;
            case SCROLLVIEW_DIR_HORIZONTAL:
                totalDis = _touchEndedPoint.x - _touchBeganPoint.x;
                if (totalDis < 0.0f)
                {
                    dir = SCROLLDIR_LEFT;
                }
                else
                {
                    dir = SCROLLDIR_RIGHT;
                }
                break;
            case SCROLLVIEW_DIR_BOTH:
            {
                CCPoint subVector = _touchEndedPoint - _touchBeganPoint;
                totalDis = subVector.getLength();
                dir = subVector.getNormalized();
                break;
            }
            default:
                break;
        }
        float orSpeed = MIN(fabs(totalDis)/(_slidTime), AUTOSCROLLMAXSPEED);
        startAutoScrollChildrenWithOriginalSpeed(dir, orSpeed, true, -1000);
        _slidTime = 0.0f;
    }
}

void ScrollView::handlePressLogic(const CCPoint &touchPoint)
{        
    _touchBeganPoint = convertToNodeSpace(touchPoint);
    _touchMovingPoint = _touchBeganPoint;    
    startRecordSlidAction();
    _bePressed = true;
}

void ScrollView::handleMoveLogic(const CCPoint &touchPoint)
{
    _touchMovedPoint = convertToNodeSpace(touchPoint);
    CCPoint delta = _touchMovedPoint - _touchMovingPoint;
    _touchMovingPoint = _touchMovedPoint;
    switch (_direction)
    {
        case SCROLLVIEW_DIR_VERTICAL: // vertical
        {
            scrollChildren(0.0f, delta.y);
            break;
        }
        case SCROLLVIEW_DIR_HORIZONTAL: // horizontal
        {
            scrollChildren(delta.x, 0.0f);
            break;
        }
        case SCROLLVIEW_DIR_BOTH: // both
        {
            scrollChildren(delta.x, delta.y);
            break;
        }
        default:
            break;
    }
}

void ScrollView::handleReleaseLogic(const CCPoint &touchPoint)
{
    _touchEndedPoint = convertToNodeSpace(touchPoint);
    endRecordSlidAction();
    _bePressed = false;
}    

bool ScrollView::onTouchBegan(CCTouch *touch, CCEvent *unusedEvent)
{
    bool pass = Layout::onTouchBegan(touch, unusedEvent);
    if (_hitted)
    {
        handlePressLogic(_touchStartPos);
    }
    return pass;
}

void ScrollView::onTouchMoved(CCTouch *touch, CCEvent *unusedEvent)
{
    Layout::onTouchMoved(touch, unusedEvent);
    handleMoveLogic(_touchMovePos);
}

void ScrollView::onTouchEnded(CCTouch *touch, CCEvent *unusedEvent)
{
    Layout::onTouchEnded(touch, unusedEvent);
    handleReleaseLogic(_touchEndPos);
}

void ScrollView::onTouchCancelled(CCTouch *touch, CCEvent *unusedEvent)
{
    Layout::onTouchCancelled(touch, unusedEvent);
    handleReleaseLogic(touch->getLocation());
}

void ScrollView::update(float dt)
{
    if (_autoScroll)
    {
        autoScrollChildren(dt);
    }
    if (_bouncing)
    {
        bounceChildren(dt);
    }
    recordSlidTime(dt);
}

void ScrollView::recordSlidTime(float dt)
{
    if (_bePressed)
    {
        _slidTime += dt;
    }
}

void ScrollView::interceptTouchEvent(int handleState, Widget *sender, const CCPoint &touchPoint)
{
    switch (handleState)
    {
        case 0:
            handlePressLogic(touchPoint);
            break;
            
        case 1:
        {
            float offset = (sender->getTouchStartPos() - touchPoint).getLength();
            if (offset > _childFocusCancelOffset)
            {
                sender->setFocused(false);
                handleMoveLogic(touchPoint);
            }
        }
            break;
            
        case 2:
            handleReleaseLogic(touchPoint);
            break;
            
        case 3:
            handleReleaseLogic(touchPoint);
            break;
    }
}

void ScrollView::checkChildInfo(int handleState,Widget* sender,const CCPoint &touchPoint)
{
    interceptTouchEvent(handleState, sender, touchPoint);
}

void ScrollView::scrollToTopEvent()
{
    if (_scrollViewEventListener && _scrollViewEventSelector)
    {
        (_scrollViewEventListener->*_scrollViewEventSelector)(this, SCROLLVIEW_EVENT_SCROLL_TO_TOP);
    }
}

void ScrollView::scrollToBottomEvent()
{
    if (_scrollViewEventListener && _scrollViewEventSelector)
    {
        (_scrollViewEventListener->*_scrollViewEventSelector)(this, SCROLLVIEW_EVENT_SCROLL_TO_BOTTOM);
    }
}

void ScrollView::scrollToLeftEvent()
{
    if (_scrollViewEventListener && _scrollViewEventSelector)
    {
        (_scrollViewEventListener->*_scrollViewEventSelector)(this, SCROLLVIEW_EVENT_SCROLL_TO_LEFT);
    }
}

void ScrollView::scrollToRightEvent()
{
    if (_scrollViewEventListener && _scrollViewEventSelector)
    {
        (_scrollViewEventListener->*_scrollViewEventSelector)(this, SCROLLVIEW_EVENT_SCROLL_TO_RIGHT);
    }
}

void ScrollView::scrollingEvent()
{
    if (_scrollViewEventListener && _scrollViewEventSelector)
    {
        (_scrollViewEventListener->*_scrollViewEventSelector)(this, SCROLLVIEW_EVENT_SCROLLING);
    }
}

void ScrollView::bounceTopEvent()
{
    if (_scrollViewEventListener && _scrollViewEventSelector)
    {
        (_scrollViewEventListener->*_scrollViewEventSelector)(this, SCROLLVIEW_EVENT_BOUNCE_TOP);
    }
}

void ScrollView::bounceBottomEvent()
{
    if (_scrollViewEventListener && _scrollViewEventSelector)
    {
        (_scrollViewEventListener->*_scrollViewEventSelector)(this, SCROLLVIEW_EVENT_BOUNCE_BOTTOM);
    }
}

void ScrollView::bounceLeftEvent()
{
    if (_scrollViewEventListener && _scrollViewEventSelector)
    {
        (_scrollViewEventListener->*_scrollViewEventSelector)(this, SCROLLVIEW_EVENT_BOUNCE_LEFT);
    }
}

void ScrollView::bounceRightEvent()
{
    if (_scrollViewEventListener && _scrollViewEventSelector)
    {
        (_scrollViewEventListener->*_scrollViewEventSelector)(this, SCROLLVIEW_EVENT_BOUNCE_RIGHT);
    }
}

void ScrollView::addEventListenerScrollView(CCObject *target, SEL_ScrollViewEvent selector)
{
    _scrollViewEventListener = target;
    _scrollViewEventSelector = selector;
}

void ScrollView::setDirection(SCROLLVIEW_DIR dir)
{
    _direction = dir;
}

SCROLLVIEW_DIR ScrollView::getDirection()
{
    return _direction;
}

void ScrollView::setBounceEnabled(bool enabled)
{
    _bounceEnabled = enabled;
}

bool ScrollView::isBounceEnabled() const
{
    return _bounceEnabled;
}

void ScrollView::setInertiaScrollEnabled(bool enabled)
{
    _inertiaScrollEnabled = enabled;
}

bool ScrollView::isInertiaScrollEnabled() const
{
    return _inertiaScrollEnabled;
}

Layout* ScrollView::getInnerContainer()
{
    return _innerContainer;
}

void ScrollView::setLayoutType(LayoutType type)
{
    _innerContainer->setLayoutType(type);
}

LayoutType ScrollView::getLayoutType() const
{
    return _innerContainer->getLayoutType();
}

void ScrollView::doLayout()
{
    if (!_doLayoutDirty)
    {
        return;
    }
    _doLayoutDirty = false;
}

std::string ScrollView::getDescription() const
{
    return "ScrollView";
}

Widget* ScrollView::createCloneInstance()
{
    return ScrollView::create();
}

void ScrollView::copyClonedWidgetChildren(Widget* model)
{
    Layout::copyClonedWidgetChildren(model);
}

void ScrollView::copySpecialProperties(Widget *widget)
{
    ScrollView* scrollView = dynamic_cast<ScrollView*>(widget);
    if (scrollView)
    {
        Layout::copySpecialProperties(widget);
        setInnerContainerSize(scrollView->getInnerContainerSize());
        setDirection(scrollView->_direction);
        setBounceEnabled(scrollView->_bounceEnabled);
        setInertiaScrollEnabled(scrollView->_inertiaScrollEnabled);
    }
}

}

NS_CC_END