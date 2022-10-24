from conans import CMake, ConanFile


class Conan(ConanFile):
    generators = "cmake_find_package", "cmake_paths"
    settings = "os", "arch", "compiler", "build_type"

    def build_requirements(self):
        self.build_requires("meson/0.63.3")

    def requirements(self):
        self.requires("glib/2.72.0", override=True)
        self.requires("libffi/3.4.2", override=True)
        self.requires("libiconv/1.17", override=True)
        self.requires("libpng/1.6.37", override=True)
        self.requires("pango/1.50.10", override=True)
        self.requires("gstreamer/1.21.1")
        self.requires("gst-libav/1.21.1")
        self.requires("gst-plugins-base/1.21.1")
        self.requires("gst-plugins-good/1.21.1")
        self.requires("gst-plugins-bad/1.21.1")
        self.requires("grpc/1.48.0")
        self.requires("cxxopts/3.0.0")
        self.requires("libjuice/1.0.5")

        if self.settings.os == "Linux":
            self.requires("expat/2.4.8", override=True)
            self.requires("wayland/1.21.0", override=True)

    def config_options(self):
        self.options['gst-plugins-good'].with_vpx = True

    def build(self):
        cmake = CMake(self)
        cmake.configure(build_folder='build')
        cmake.build()
