#ifndef RENDER_COMMAND
#define RENDER_COMMAND(...)
#pragma message(error,  "RENDER_COMMAND macro not defined, invalid use of '#include \"opengl_render_command_list.h\"'")
#endif

RENDER_COMMAND(enable);
RENDER_COMMAND(disable);
RENDER_COMMAND(clear_color);
RENDER_COMMAND(clear);
RENDER_COMMAND(blend_equation);
RENDER_COMMAND(blend_factors);
RENDER_COMMAND(scissor_test);
RENDER_COMMAND(bind_render_target);
RENDER_COMMAND(copy_render_target);
RENDER_COMMAND(bind_vertex_array);
RENDER_COMMAND(use_program_pipeline);
RENDER_COMMAND(use_program);

RENDER_COMMAND(active_texture);
RENDER_COMMAND(bind_texture);

RENDER_COMMAND(bind_buffer);
RENDER_COMMAND(buffer_data);
RENDER_COMMAND(buffer_data_ptr);
RENDER_COMMAND(bind_vertex_buffer);
RENDER_COMMAND(bind_buffer_range);

RENDER_COMMAND(draw_elements);
RENDER_COMMAND(draw_elements_base_vertex);
RENDER_COMMAND(draw_elements_instanced);
RENDER_COMMAND(draw_elements_instanced_base_vertex);
RENDER_COMMAND(draw_elements_instanced_base_vertex_base_instance);

