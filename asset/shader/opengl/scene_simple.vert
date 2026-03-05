#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 3) in ivec4 in_bone_ids;
layout(location = 4) in vec4 in_bone_weights;

out vec3 vert_normal;
out vec3 vert_position;
out vec2 vert_texcoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform int skinning_enabled;
uniform mat4 bone_matrices[128];

void main()
{
	vec4 local_position = vec4(in_position, 1.0);
	vec3 local_normal = in_normal;
	if (skinning_enabled != 0)
	{
		mat4 skin =
			in_bone_weights.x * bone_matrices[in_bone_ids.x] +
			in_bone_weights.y * bone_matrices[in_bone_ids.y] +
			in_bone_weights.z * bone_matrices[in_bone_ids.z] +
			in_bone_weights.w * bone_matrices[in_bone_ids.w];
		local_position = skin * local_position;
		local_normal = mat3(skin) * local_normal;
	}
	vert_normal = normalize(mat3(model) * local_normal);
	vert_texcoord = in_texcoord;
	mat4 pvm = projection * view * model;
	vert_position = (pvm * local_position).xyz;
	gl_Position = pvm * local_position;
}
