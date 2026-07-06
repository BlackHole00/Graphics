package main

import "core:log"
import "core:math"
import la "core:math/linalg"

main :: proc() {
	context.logger = log.create_console_logger()
	defer log.destroy_console_logger(context.logger)

	q_identity: quaternion128 = 1
	direction := la.mul(q_identity, [3]f32{0, 0, 1})
	right := la.mul(q_identity, [3]f32{1, 0, 0})
	log.info(q_identity, direction, right)

	up := la.quaternion_angle_axis(-math.PI / 2, [3]f32{1, 0, 0})
	direction = la.mul(up, [3]f32{0, 0, 1})
	right = la.mul(up, [3]f32{1, 0, 0})
	log.info(up, direction, right)

	deg45 := la.quaternion_angle_axis(math.PI/4, [3]f32{0, 1, 0})
	direction = la.mul(deg45, [3]f32{0, 0, 1})
	right = la.mul(deg45, [3]f32{1, 0, 0})
	log.info(deg45, direction, right)

	up_deg45 := deg45 * up
	direction = la.mul(up_deg45, [3]f32{0, 0, 1})
	right = la.mul(up_deg45, [3]f32{1, 0, 0})
	log.info(up_deg45, direction, right)

	actual_rotation := la.quaternion_from_euler_angles_f32(
		-math.PI / 4,
		 math.PI / 4,
		0,
		.YXZ,
	)
	direction = la.mul(actual_rotation, [3]f32{0, 0, 1})
	right = la.mul(actual_rotation, [3]f32{1, 0, 0})
	log.info(actual_rotation, direction, right)
}
