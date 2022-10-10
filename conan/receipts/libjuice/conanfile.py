from conans import ConanFile, CMake, tools


class LibjuiceConan(ConanFile):
    name = "libjuice"
    license = "LGPL-2.1"
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "https://github.com/paullouisageneau/libjuice"
    description = "libjuice is a UDP Interactive Connectivity Establishment" \
                  "library allows to open bidirectionnal User Datagram " \
                  "Protocol (UDP) streams with Network Address Translator " \
                  "(NAT) traversal."
    topics = ("p2p", "udp", "stun")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "cmake"
    exports_sources = ["CMakeLists.txt"]

    @property
    def _src_subfolder(self):
        return "source_subfolder"

    @property
    def _build_subfolder(self):
        return "build_subfolder"

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.configure(build_folder=self._build_subfolder)
        return cmake

    def _cmake_target(self):
        return "juice-static"  # "juice" if self.options.shared else "juice-static"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def source(self):
        tools.get(**self.conan_data["sources"][self.version],
                  destination=self._src_subfolder, strip_root=True)

    def build(self):
        cmake = self._configure_cmake()
        cmake.build(target=self._cmake_target())

    def package(self):
        self.copy(pattern="LICENSE", dst="licenses", src=self._src_subfolder)
        cmake = self._configure_cmake()
        cmake.install()
        if not self.options.shared:
            self.copy("lib/libjuice-static.a", dst=".", src=self._build_subfolder)

    def package_info(self):
        if self.options.shared:
            self.cpp_info.libs = ["juice"]
        else:
            self.cpp_info.libs = ["juice-static"]
