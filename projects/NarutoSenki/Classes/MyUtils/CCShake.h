#pragma once
#include "Defines.h"

class CCShake : public ActionInterval
{
	// Code by Francois Guibert
	// Contact: www.frozax.com - http://twitter.com/frozax - www.facebook.com/frozax
public:
	CCShake();

	bool initWithDuration(float d, float strength_x, float strength_y);

	// Create the action with a time and a strength (same in x and y)
	static CCShake *create(float d, float strength);
	// Create the action with a time and strengths (different in x and y)
	static CCShake *createWithStrength(float d, float strength_x, float strength_y);

protected:
	void startWithTarget(Node *pTarget);
	void update(float time);
	void stop();

	// Initial position of the shaked node
	float m_initial_x, m_initial_y;
	// Strength of the action
	float m_strength_x, m_strength_y;
};
