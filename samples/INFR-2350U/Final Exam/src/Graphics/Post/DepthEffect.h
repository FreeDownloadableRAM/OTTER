#pragma once

#include "Graphics/Post/PostEffect.h"

class DepthEffect : public PostEffect
{
public:
	//Initializes the framebuffer with extra steps
	//*Sets size
	//*Initializes the framebuffer afterwards
	void Init(unsigned width, unsigned height) override;

	//Applies the affect to this screen
	//*Passes the framebuffer with the texture to apply as a parameter
	void ApplyEffect(PostEffect* buffer) override;

	//Reshapes the buffers
	void Reshape(unsigned width, unsigned height) override;

	//Getters
	float GetDownscale() const;
	float GetThreshold() const;
	float GetMinimum() const;
	float GetMaximum() const;
	unsigned GetPasses() const;

	//Setters
	void SetDownscale(float downscale);
	void SetThreshold(float threshold);
	void SetMinimum(float minimum);
	void SetMaximum(float maximum);
	void SetPasses(unsigned passes);

private:
	float _downscale = 1.f;
	float _threshold = 0.01f;
	float _minimum = 1.0f;
	float _maximum = 3.0f;
	unsigned _passes = 10;

	//make glm
	glm::vec2 _pixelSize;

};



