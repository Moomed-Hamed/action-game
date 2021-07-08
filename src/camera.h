#include "renderer.h"

#define DIR_FORWARD	0
#define DIR_BACKWARD	1
#define DIR_LEFT	2
#define DIR_RIGHT	3

struct Camera
{
	vec3 position;
	vec3 front, right, up;
	float yaw, pitch;
};

void camera_update_dir(Camera* camera, float dx, float dy, float sensitivity = 0.003)
{
	camera->yaw   += (dx * sensitivity) / TWOPI;
	camera->pitch += (dy * sensitivity) / TWOPI;

	if (camera->pitch >  PI / 2.01) camera->pitch =  PI / 2.01;
	if (camera->pitch < -PI / 2.01) camera->pitch = -PI / 2.01;

	camera->front.y = sin(camera->pitch);
	camera->front.x = cos(camera->pitch) * cos(camera->yaw);
	camera->front.z = cos(camera->pitch) * sin(camera->yaw);

	camera->front = normalize(camera->front);
	camera->right = normalize(cross(camera->front, vec3(0, 1, 0)));
	camera->up    = normalize(cross(camera->right, camera->front));
}
void camera_update_pos(Camera* camera, int direction, float distance)
{
	if (direction == DIR_FORWARD ) camera->position += camera->front * distance;
	if (direction == DIR_LEFT    ) camera->position -= camera->right * distance;
	if (direction == DIR_RIGHT   ) camera->position += camera->right * distance;
	if (direction == DIR_BACKWARD) camera->position -= camera->front * distance;
}