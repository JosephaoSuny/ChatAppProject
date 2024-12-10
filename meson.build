project('chatAppV2', 'cpp',
        version : '1.0.0',
        default_options : ['warning_level=3', 'cpp_std=c++20'])

glfw = dependency('glfw3')
opengl = dependency('opengl')
imgui = dependency('imgui')

chatAppV2 = executable('chatAppV2', ['main.cpp', 'shared.cpp'], install : true, dependencies: [imgui, glfw, opengl])
chatClientV2 = executable('chatClientV2', ['client.cpp', 'shared.cpp'], install : true, dependencies: [imgui, glfw, opengl])

test('test', chatAppV2)