workspace "sln-nc"
  objdir "builddir/obj"
  targetdir "builddir"
  libdirs { "builddir" }

  configurations { "Debug", "Release" }

  configuration "Debug"
     defines { "DEBUG" }
     symbols "On"

  configuration "Release"
     defines { "NDEBUG" }
     optimize "On"

  project "nc"
    kind "StaticLib"
    language "C"
    files {
      "src/*.c",
    }
    includedirs {
      "src",
    }

    filter { "system:windows", "toolset:gcc" }
      defines { "CRT_MINGW", "MINGW_HAS_SECURE_API", "_POSIX_C_SOURCE" }
      buildoptions { }

  project "test"
    kind "ConsoleApp"
    language "C"

    files {
      "tests/*.c",
    }
    includedirs {
      "src",
      "tests",
    }
    libdirs {
      "builddir",
    }

    filter "system:windows"
      defines { "CRT_MINGW", "MINGW_HAS_SECURE_API", "_POSIX_C_SOURCE" }
      links { "nc" }
      linkoptions { "-Wall" }
