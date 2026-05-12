#include "Effect.h"
#include "CharacterBase.h"
#include "GameLayer.h"
#include <functional>
#include <unordered_map>
#include <unordered_set>

bool Effect::init(const string &name, CharacterBase *attacker)
{
	RETURN_FALSE_IF(!Sprite::init());

	at = attacker;
	if (!at)
		return false;

	auto runAnim = [&](const string &prefix, size_t frameCount, uint8_t fps, bool isLoop)
	{
		runAction(createEffectAnimation(prefix, frameCount, fps, isLoop));
	};

	// hit effects
	static const std::unordered_set<string> kHitMinor = {"n_hit", "b_hit"};
	static const std::unordered_set<string> kHitMajor = {"l_hit", "f_hit", "f2_hit", "c_hit", "o_hit", "o2_hit", "ob_hit", "ab_hit", "ac_hit", "ct_hit", "ts_hit", "s_hit"};
	static const std::unordered_set<string> kHitBlue = {"bl_hit", "bc_hit", "bf_hit", "sl_hit"};

	if (kHitMinor.count(name) > 0)
	{
		runAnim("red_damge_", 3, 14, false);
		setAnchorPoint(Vec2(0.5f, 0));
		setScale(0.6f);
		setPosition(Vec2(at->getPositionX() + rand() % 4, at->getPositionY() + rand() % 16));
		return true;
	}
	if (kHitMajor.count(name) > 0)
	{
		runAnim("red_damge_", 3, 14, false);
		setScale(0.8f);
		setAnchorPoint(Vec2(0.5f, 0));
		setPosition(at->getPosition());
		return true;
	}
	if (kHitBlue.count(name) > 0)
	{
		runAnim("blue_damge_", 3, 14, false);
		setScale(0.6f);
		setRotation(rand() % 30 + 10);
		setPosition(Vec2(at->getPositionX(), at->getPositionY() + at->getContentSize().height / 2));
		return true;
	}

	// categorized registry for non-hit effects
	const std::unordered_map<string, std::function<void()>> kEffectRegistry = {
		{"a_hit", [&]() { runAnim("bottom_damage_", 5, 20, false); setAnchorPoint(Vec2(0.5f, 0)); setPosition(Vec2(at->getPositionX() + 16, at->getPositionY())); }},
		{"Kagura", [&]() { runAnim("Kagura_", 8, 10, false); }},
		{"Hupo", [&]() { runAnim("Hupo_", 6, 10, false); }},
		{"hp_restore", [&]() { auto seq = newSequence(createEffectAnimation(name + "_", 4, 5, false), CallFunc::create(std::bind(&CharacterBase::disableEffect, at))); runAction(seq); }},
		{"speedUp", [&]() { auto seq = newSequence(createEffectAnimation(name + "_", 4, 5, false), CallFunc::create(std::bind(&CharacterBase::disableEffect, at))); runAction(seq); }},
		{"smk", [&]() { runAnim("smk_", 4, 10, false); setAnchorPoint(Vec2(0.5f, 0)); setPosition(at->getPosition()); if (UserDefault::sharedUserDefault()->getBoolForKey("isVoice")) SimpleAudioEngine::sharedEngine()->playEffect("Audio/Effect/poof.wav"); }},
		{"tishen", [&]() { runAnim("tishen_", 6, 10, false); setAnchorPoint(Vec2(0.5f, 0)); setPosition(at->getPosition()); }},
		{"stun", [&]() { runAnim("stun_", 4, 5, false); setAnchorPoint(Vec2(0.5f, 0)); setPosition(Vec2(at->getPositionX(), at->getPositionY() + at->getContentSize().height - 2)); }},
		{"DarkFlame", [&]() { runAnim("DarkFlame_Effect_", 4, 10, true); setAnchorPoint(Vec2(0.5f, 0)); setPosition(at->getPosition()); runAction(newSequence(DelayTime::create(2.8f), CallFunc::create(std::bind(&Effect::removeEffect, this)))); }},
		{"Bagua", [&]() { runAnim("Bagua_", 23, 10, false); setAnchorPoint(Vec2(0.5f, 0)); setPosition(Vec2(at->getPositionX() + 2, at->getPositionY() - 52)); }},
		{"Kujiyose", [&]() { initWithSpriteFrameName(name.c_str()); setAnchorPoint(Vec2(0.5f, 0)); setPosition(Vec2(at->getPositionX(), at->getPositionY() - getContentSize().height / 2)); runAction(newSequence(DelayTime::create(0.3f), CallFunc::create(std::bind(&Effect::removeEffect, this)))); }},
		{"kazi", [&]() { initWithSpriteFrameName(name.c_str()); setAnchorPoint(Vec2(0.5f, 0)); setPosition(Vec2(at->getPositionX() + (at->_isFlipped ? -32 : 32), at->getPositionY() + at->getContentSize().height / 2)); auto su = ScaleBy::create(0.1f, 1.2f); runAction(newSequence(su, su->reverse(), CallFunc::create(std::bind(&Effect::removeEffect, this)))); }},
		{"hBuff", [&]() { runAnim("hBuff_Effect_", 3, 5, true); setAnchorPoint(Vec2(0.5f, 0)); }},
		{"sBuff", [&]() { runAnim("sBuff_Effect_", 10, 10, true); setAnchorPoint(Vec2(0.5f, 0)); }},
		{"hsBuff", [&]() { runAnim("hsBuff_Effect_", 12, 10, true); setAnchorPoint(Vec2(0.5f, 0)); }},
		{"tBuff", [&]() { runAnim("tBuff_Effect_", 12, 10, true); setAnchorPoint(Vec2(0.5f, 0)); }},
		{"dcBuff", [&]() { runAnim("dcBuff_Effect_", 10, 10, true); setAnchorPoint(Vec2(0.5f, 0)); }},
		{"jdBuff", [&]() { runAnim("jdBuff_Effect_", 8, 10, true); setAnchorPoint(Vec2(0.5f, 0)); }},
		{"bmBuff", [&]() { runAnim("bmBuff_Effect_", 8, 10, true); setAnchorPoint(Vec2(0.5f, 0)); }},
		{"dhBuff", [&]() { for (int i = 0; i < 3; i++) { auto ef = Sprite::createWithSpriteFrameName("FireEffect_01"); ef->setAnchorPoint(Vec2(0.5f, 0)); ef->setPosition(i == 0 ? Vec2(10, 0) : (i == 1 ? Vec2(-10, 18) : Vec2(10, 34))); ef->runAction(createEffectAnimation("FireEffect_", 4, 10, true)); addChild(ef); } }},
	};

	// special aliases mapped before fallback
	if (name == "sharingan" || name == "sharingan2" || name == "sharingan3" || name == "sharingan4")
	{
		initWithSpriteFrameName(name.c_str());
		setPosition(Vec2(at->getPositionX() + (at->_isFlipped ? -32 : 32), at->getPositionY() + at->getContentSize().height));
		auto rt = RotateBy::create(0.3f, 180, 180);
		auto su = ScaleBy::create(0.2f, 1.6f);
		runAction(newSequence(rt, su, CallFunc::create(std::bind(&Effect::removeEffect, this))));
		return true;
	}
	if (name == "Hiraishin" || name == "Hiraishin2")
	{
		runAnim(name + "_Effect_", 3, 10, false);
		setAnchorPoint(Vec2(0.5f, 0));
		setPosition(at->getPosition());
		return true;
	}

	auto it = kEffectRegistry.find(name);
	if (it != kEffectRegistry.end())
	{
		it->second();
		return true;
	}

	CCLOG("[EffectRegistry] Create effect failed by name %s", name.c_str());
	return false;
}

FiniteTimeAction *Effect::createEffectAnimation(const string &file, size_t frameCount, uint8_t fps, bool isLoop)
{
	Vector<SpriteFrame *> spriteFrames(frameCount);

	for (size_t i = 1; i < frameCount + 1; i++)
	{
		auto frame = getSpriteFrame("{}{:02d}", file, i);
		spriteFrames.pushBack(frame);

		// NOTE: For cocos2d-x 2.x using SpriteBatchNode,
		// the sprite must be initialized before SpriteBatchNode::addChild
		// Otherwise you will get an empty texture error
		if (i == 1)
			initWithSpriteFrame(frame);
	}

	auto animation = Animation::createWithSpriteFrames(spriteFrames, 1.0 / fps);
	auto animAction = Animate::create(animation);

	if (isLoop)
	{
		return RepeatForever::create(animAction);
	}
	else
	{
		auto call = CallFunc::create(std::bind(&Effect::removeEffect, this));
		return newSequence(animAction, call);
	}
}

FiniteTimeAction *Effect::createFontAnimation()
{
	auto delay = DelayTime::create(0.3f);
	auto call = CallFunc::create(std::bind(&Effect::removeFontEffect, this));
	auto seq = newSequence(delay, call);
	return seq;
}

void Effect::removeEffect()
{
	if (at->damageEffectCount > 0)
	{
		at->damageEffectCount--;
	}
	stopAllActions();
	removeFromParent();
}

void Effect::removeFontEffect()
{
	stopAllActions();
	removeFromParent();
}

Effect *Effect::create(const string &szImage, CharacterBase *attacker)
{
	Effect *ef = new Effect();
	if (ef && ef->init(szImage, attacker))
	{
		ef->autorelease();
		return ef;
	}
	else
	{
		delete ef;
		return nullptr;
	}
}
