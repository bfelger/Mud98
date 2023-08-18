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
- Builds without errors or warnings in GCC, Clang, and MSVC.
- Continuing implementation of modern C (including newer C standards).
- Secure telnet with TLS and password digests provided by OpenSSL.

Other than the improvements listed above, Mud98 intends to be true to the spirit of the original ROM as possible; to be a _tabula rasa_ on which other, more modern MUD's can be built.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- GETTING STARTED -->
## Getting Started

This is an example of how you may give instructions on setting up your project locally.
To get a local copy up and running follow these simple example steps.

### Prerequisites

The following libraries and utilities are required to build Mud98:
* CMake 3.10 or higher.
* OpenSSL 3.x.
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
   ./config && ./build
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
        "-a",
        "../../../../area/"
      ]
    ```

    This will add the launch option, "Mud98.exe [with args]" that will run Mud98 from the `area` directory.

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
./Mud98 --area=$MUD98_AREA_DIR --port=$MUD98_PORT
```

Replace the variables as necessary.

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

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".
Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- LICENSE -->
## License

Distributed under the Diku, Merc, and ROM licenses. See `LICENSE.txt` for more information.

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
