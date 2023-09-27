<a name="readme-top"></a>

[![Contributors][contributors-shield]][contributors-url]
[![Issues][issues-shield]][issues-url]
[![MIT License][license-shield]][license-url]

<br />
<div align="center">
  <a href="https://github.com/bfelger/Mud98">
    <img src="doc/Mud98.png" alt="Logo" width="613" height="596">
  </a>

  <h3 align="center">Mud98 v0.9</h3>

  <p align="center">
    A classic MUD (Multi-User Dungeon) updated for a new(ish) millennium
    <!--br />
    <a href="https://github.com/bfelger/Mud98"><strong>Explore the docs »</strong></a-->
    <br />
    <br />
    <a href="https://github.com/bfelger/Mud98/issues">Report Bug</a>
    ·
    <a href="https://github.com/bfelger/Mud98/issues">Request Feature</a>
  </p>
</div>

<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-mud98">About Mud98</a>
    </li>
    <li>
      <a href="#features">Features</a>
      <ul>
        <li><a href="#tls-secure-sockets">TLS Secure Sockets</a></li>
        <li><a href="#file-based-configuration">File-based Configuration</a></li>
        <li><a href="#improved-code-quality">Improved Code Quality</a></li>
        <li><a href="#olc-expansion">OLC Expansion</a></li>
        <li><a href="#24-bit-color-themes">24-bit Color Themes</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#building-mud98">Building Mud98</a></li>
      </ul>
    </li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ol>
</details>

<!-- ABOUT MUD98 -->
## About Mud98

The last official release of Rivers of MUD II (ROM) was 2.4b6, circa 1998. It has been the basis of countless other MUDs over the last couple decades. Over those years, however, it has gotten increasingly more difficult to build with modern compilers without numorous errors and warnings. This project aims to be a viable starting point for future MUDs by updating the code base to modern standards.

Mud98 has the following improvements:

- ROM 2.4b6 base with Lope's ColoUr and OLC 2.1.
- Cross-platform compilation with CMake.
- Builds without errors or warnings in GCC, Clang, MSVC, and Cygwin.
  - Compiles with `-Wall -Werror -Wextra -pedantic`.
- Continuing implementation of modern C (including newer C standards).
- Secure telnet with TLS and password digests provided by OpenSSL.
- User-defined, shareable color themes.

Other than the improvements listed above, Mud98 intends to be true to the spirit of the original ROM as possible; to be a _tabula rasa_ on which other, more modern MUD's can be built.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- FEATURES -->
## Features

Here are some of the novel improvements to Mud98 over stock ROM:

<!-- TLS Secure Sockets -->
### TLS Secure Sockets

OpenSSL provides secure sockets to Mud98. You can configure Mud98 to run TLS in tandem with clear-text telnet, or by itself.

<!-- File-base Configuration -->
### File-based Configuration

Most settings in Mud98 can be configured in `mud98.cfg`, which resides in the root folder. This includes telnet/TLS settings, file settings, and a growing list of gameplay customization options.

<!-- Improved Code Quality -->
### Improved Code Quality

Part of an on-going effort, Mud98 applies "modern" C best practices to ROM's legacy C code. This is a multi-prong effort:
- Reorganization for smaller, focused code files with an eye toward Single Responsibility Principle.
- Scrupulous application of more constistent (though admittedly opinionated) naming, syntax, and structure.
- Removal of legacy code that is unlikely to ever see usage on modern systems.

<!-- OLC Expansion -->
### OLC Expansion

Mud98 adds a class editor (`cedit`) to OLC 2 for ROM. Future plans are to expand on this and add more features editable via OLC.

<!-- 24-BIT COLOR THEMES -->
### 24-Bit Color Themes

Players can create new color themes and share them publicly with other players. Mud98 supports traditional ANSI, extended 256 color, and 24-bit true color. True color support covers both standard SGR format (Mudlet) and `xterm` format (TinTin++). 

<div>
  <a href="doc/theme-1.png">
  <img src="doc/theme-1.png" alt="Logo" width="295" height="362">
  </a> <a href="doc/theme-2.png">
  <img src="doc/theme-2.png" alt="Logo" width="301" height="280">
  </a>
</div>

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- GETTING STARTED -->
## Getting Started

You can build and run Mud98 on virtually any desktop or server platform.

### Prerequisites

The following libraries and utilities are required to build Mud98:
* CMake 3.12 or higher.
* Pthreads (Linux, Cygwin)

The following are optional, but **highly** recommended:
* OpenSSL 3.x+.
  - Required for TLS server capability.
* A certificate signed by a legitimate Certificate Authority (if you don't want users getting warnings about self-signed certificates).

### Building Mud98

1. Clone the repo
   ```sh
   git clone https://github.com/bfelger/Mud98.git
   ```
2. Place an SSL key and signed certificate in the `keys` folder

   **or**

   Generate self-signed SSL keys _(for local dev purposes, only!)_
   ```sh
   cd keys && ./ssl_gen_keys
   ```
3. Configure CMake and build

    **For GCC or Clang under Linux or Cygwin:**
   ```sh
   cd src
   ./config && ./compile
   ```

    **For Visual Studio (MSVC)**:
    
    Open the `src` folder in Visual Studio with CMake support enabled.

    You may need to manually set the following CMake properties:
    - `OPENSSL_INCLUDE_DIR`: _`<OpenSSLDir>`_`/include`
    - `SSL_EAY_DEBUG`/`RELEASE`: _`<OpenSSLDir>`_`/lib/libssl.lib`

    To run Mud98 from the IDE, you need to add the following launch configuration to `src/.vs/launch.vs.json`:

    ```json
      "type": "default",
      "project": "CMakeLists.txt",
      "projectTarget": "Mud98.exe",
      "name": "Mud98.exe [with args]",
      "args": [
        "-d",
        "../../../../"
      ]
    ```

    This will add the launch option, "Mud98.exe [with args]" that will run Mud98 from the root directory.

    If you wish, you can still run Mud98 from the `area` directory as was custom in ROM.

    **For Windows CLI (MSVC)**

    You need to know where `cmake.exe` lives. In my case, for instance, for Visual Studio 2022 Community Edition, it would be here:

    ```
    C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe
    ```
    You can then configure and build from the command line:

    ```
    cmake.exe -G "Ninja Multi-Config" -B out
    cmake --build out --clean-first --config Debug
    ```

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- USAGE EXAMPLES -->
## Usage

Once Mud98 is built, it can be run from the command-line like so:

```sh
./Mud98
```

Use the `-d` argument to specify the folder to `mud98.cfg` if you don't run it from the base folder.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- ROADMAP 
## Roadmap

- [ ] Feature 1
- [ ] Feature 2
- [ ] Feature 3
- [ ] Nested Feature

See the [open issues](https://github.com/github_username/repo_name/issues) for a full list of proposed features (and known issues).

<p align="right">(<a href="#readme-top">back to top</a>)</p-->

<!-- CONTRIBUTING -->
## Contributing

Please, please feel free to add this project. Or take it and do something completely different with it. ROM was responsible for my learning to love code in my youth, and I believe it yet has more to teach. Make it your own!

How to contribute:

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- LICENSE -->
## License

Distributed under the Diku, Merc, and ROM licenses. See `LICENSE.txt` for more information. I make no assumptions as to their enforceability (or even the feasability of compliance); use at your own risk.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- CONTACT -->
## Contact

Brandon Felger - bfelger@gmail.com

Project Link: [https://github.com/bfelger/Mud98](https://github.com/bfelger/Mud98)

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- ACKNOWLEDGMENTS 
## Acknowledgments

* []()
* []()
* []()

<p align="right">(<a href="#readme-top">back to top</a>)</p-->

<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[contributors-shield]: https://img.shields.io/github/contributors/bfelger/Mud98.svg?style=for-the-badge
[contributors-url]: https://github.com/bfelger/Mud98/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/bfelger/Mud98.svg?style=for-the-badge
[forks-url]: https://github.com/bfelger/Mud98/network/members
[stars-shield]: https://img.shields.io/github/stars/bfelger/Mud98.svg?style=for-the-badge
[stars-url]: https://github.com/bfelger/Mud98/stargazers
[issues-shield]: https://img.shields.io/github/issues/bfelger/Mud98.svg?style=for-the-badge
[issues-url]: https://github.com/bfelger/Mud98/issues
[license-shield]: https://img.shields.io/github/license/bfelger/Mud98.svg?style=for-the-badge
[license-url]: https://github.com/bfelger/Mud98/blob/master/LICENSE.txt
