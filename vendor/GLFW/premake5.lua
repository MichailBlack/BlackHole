project "GLFW"
    kind "StaticLib"
    language "C"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "include/GLFW/glfw3.h",
        "include/GLFW/glfw3native.h",
        "src/context.c",
        "src/egl_context.c",
        "src/glx_context.c",
        "src/init.c",
        "src/input.c",
        "src/internal.h",
        "src/mappings.h",
        "src/monitor.c",
        "src/osmesa_context.c",
        "src/platform.c",
        "src/platform.h",
        "src/vulkan.c",
        "src/wgl_context.c",
        "src/window.c",
        "src/null_init.c",
        "src/null_joystick.c",
        "src/null_joystick.h",
        "src/null_monitor.c",
        "src/null_platform.h",
        "src/null_window.c",
    }

    filter "system:windows"
        cdialect "C17"
        systemversion "latest"
        staticruntime "On"

        files
        {
            "src/win32_init.c",
            "src/win32_joystick.c",
            "src/win32_joystick.h",
            "src/win32_module.c",
            "src/win32_monitor.c",
            "src/win32_platform.h",
            "src/win32_thread.c",
            "src/win32_thread.h",
            "src/win32_time.c",
            "src/win32_time.h",
            "src/win32_window.c"
        }

        defines
        {
            "_GLFW_WIN32",
            "_CRT_SECURE_NO_WARNINGS"
        }

    filter { "system:windows", "configurations:Release" }
        buildoptions "/MT"