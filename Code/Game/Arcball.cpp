#include "precompiled.h"
#include "Arcball.h"

namespace ArcBall{
	cArcBall::cArcBall(void)
		: is_dragged_(false),
		radius_(1.0f),
		previous_point_(D3DXVECTOR3(0, 0, 0)),
		current_point_(D3DXVECTOR3(0, 0, 0))
	{
		D3DXMatrixIdentity(&increment_matrix_);
		D3DXMatrixIdentity(&rotate_matrix_);

		RECT rc;
		GetClientRect(GetForegroundWindow(), &rc);

		int window_width = rc.right - rc.left;
		int window_height = rc.bottom - rc.top;

		SetWindow(window_width, window_height);
	}

	cArcBall::~cArcBall(void)
	{
	}

	void cArcBall::Reset()
	{
		D3DXMatrixIdentity(&increment_matrix_);
		D3DXMatrixIdentity(&rotate_matrix_);
		is_dragged_ = false;
		radius_ = 1.0f;
	}

	void cArcBall::OnBegin(int mouse_x, int mouse_y)
	{
		// enter drag state only if user click the window's client area
		if (mouse_x >= 0 && mouse_x <= window_width_
			&& mouse_y >= 0 && mouse_y < window_height_)
		{
			is_dragged_ = true; // begin drag state
			previous_point_ = ScreenToVector(mouse_x, mouse_y);
		}
	}

	void cArcBall::OnMove(int mouse_x, int mouse_y)
	{
		if (is_dragged_)
		{
			current_point_ = ScreenToVector(mouse_x, mouse_y);
			D3DXMATRIX yRotation;
			D3DXMatrixIdentity(&yRotation);
			D3DXMatrixRotationY(&yRotation, current_point_.x - previous_point_.x);
			D3DXMATRIX xRotation;
			D3DXMatrixIdentity(&xRotation);
			D3DXMatrixRotationX(&xRotation, -(current_point_.y - previous_point_.y));
			D3DXMATRIX zRotation;
			D3DXMatrixIdentity(&zRotation);
			D3DXMatrixMultiply(&increment_matrix_, &yRotation, &xRotation);
			D3DXMatrixMultiply(&increment_matrix_, &increment_matrix_, &zRotation);
			D3DXMatrixMultiply(&rotate_matrix_, &rotate_matrix_, &increment_matrix_);
			//rotate_matrix_._11 = 1;
			rotate_matrix_._12 = 0;
			rotate_matrix_._21 = 0;
			//rotate_matrix_._22 = 1;
			previous_point_ = current_point_;
		}
	}

	void cArcBall::OnEnd()
	{
		is_dragged_ = false;
	}

	void cArcBall::SetWindow(int window_width, int window_height, float arcball_radius)
	{
		window_width_ = window_width;
		window_height_ = window_height;
		radius_ = arcball_radius;
	}

	const D3DXMATRIX* cArcBall::GetRotationMatrix()
	{
		D3DXMATRIX temp;
		D3DXMatrixIdentity(&temp);
		return &rotate_matrix_;
	}

	D3DXQUATERNION cArcBall::QuatFromBallPoints(D3DXVECTOR3& start_point, D3DXVECTOR3& end_point)
	{
		// Calculate rotate angle
		float angle = D3DXVec3Dot(&start_point, &end_point);

		// Calculate rotate axis
		D3DXVECTOR3 axis;
		D3DXVec3Cross(&axis, &start_point, &end_point);

		// Build and Normalize the Quaternion
		D3DXQUATERNION quat(axis.x, axis.y, axis.z, angle);
		D3DXQuaternionNormalize(&quat, &quat);

		return quat;
	}

	D3DXVECTOR3 cArcBall::ScreenToVector(int screen_x, int screen_y)
	{
		// Scale to screen
		float x = -(screen_x - window_width_ / 2) / (radius_ * window_width_ / 2);
		float y = (screen_y - window_height_ / 2) / (radius_ * window_height_ / 2);

		float z = 0.0f;
		float mag = x * x + y * y;

		if (mag > 1.0f)
		{
			float scale = 1.0f / sqrtf(mag);
			x *= scale;
			y *= scale;
		}
		else
			z = sqrtf(1.0f - mag);

		return D3DXVECTOR3(x, y, z);
	}

}
