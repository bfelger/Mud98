# Worldcrafting from Scratch Pt. 1 &mdash; Getting Started

### Table of Contents
1. [Building Mud98](#building-mud98)
    - [Prerequisites](#prerequisites)
    - [Building on Linux and Cygwin](#building-on-linux-and-cygwin)
    - [Building on Windows](#building-on-windows)
2. [Configuring Mud98](#configuring-mud98)
    - [TLS certificates](#tls-certificates)
    - [Edit mud98.cfg](#edit-mud98cfg)
3. [Booting Mud98](#booting-mud98)
4. [Creating an implementor](#creating-an-implementor)

<br />

Mud98 isn't a MUD; it's a MUD _codebase_. The task of crafting a MUD around it is up to you. The original area files are provided if you want to experience ROM at is was back in 1998; but they aren't very serious or cohesive, and there's a good chance you don't actually want the 1950's diner in Midgaard in your fantasy-themed setting.

This document aims to create a step-by-step how-to to take a brand-new, vanilla Mud98, and build a brand-new MUD around it that is 100% yours.

Be before we can do any of that, we have to get the thing up and running. I have done my due diligence to make sure that no matter what environment you're on, whether it's Linux, Windows, or even Cygwin, Mud98 compiles and executes flawlessly.

To give you some idea of where things have been with ROM since its last release, the code would not compile with out an unbelievable number of errors and warnings,and had to be compiled with some pretty old (and out-dated) legacy compatability settings.

Now it compiles on all major C compilers without any of those problems so that you may not have to get a Computer Science degree just to learn how to build your own MUD.

> I joke; I learned _way_ more from hacking on ROM than I ever did my Comp Sci degree.

## Building Mud98

If you're reading this, you presumedly have the source code already downloaded. If not, head on over to [the GitHub repo](https://github.com/bfelger/Mud98/) and download it.

You can also download it from command-line:

```sh
$ git clone https://github.com/bfelger/Mud98.git
```

### Prerequisites

These are the things you need to build Mud98:

#### Compiler that supports at least C99

The CMake scripts will actually try to use C23 (on Linux) or C17 (on Windows). Either way, most of the novel updates to the ROM codebase in Mud98 require C99.

I have tested and verified that Mud98 builds without error and executes without problems on the following:

- GCC 11.3 (Ubuntu)
- Clang 14 (Ubuntu)
- MSVC 19.37 (Visual Studio 22)
- GCC 11.4 (Cygwin)

I have not tested Mud98 under production load on Windows. Theoretically is _should_ be fine; the original ROM codebase is far more copacetic with modern Microsoft than it was in the MS-DOS days.

#### CMake 3.12 or higher

I chose to leave behind the convenience and ease-of-use of conventional Makefiles because of the absolute ease of cross-platform configuration CMake brings. Writing CMake scripts isn't easy, at all; but hopefully I've done the hard work for you.

#### Pthreads library (Linux & Cygwin only)

Because of the necessarily parallelized nature of socket accepts (to facilitate long SSL handshakes), this portion of Mud98 is multithreaded. In MSVC, the multithreading code uses native Win32 calls. On Linux and Cygwin, it uses Pthreads. Many Linux distros will include it when installing development packages. Cygwin will need to have it selected explicitly in the installer.

#### OpenSSL 3.0 or higher

Strictly speaking, this is _optional_; but Mud98 will be so degraded in security that I do not advise it.

In Mud98, OpenSSL provides secure TLS connection as well as SHA256 hashing for passwords.

> A note about OpenSSL: there are _no official builds or downloads_. Essentially, the only OpenSSL binary  you can trust is the one you build, yourself. It isn't especially difficult, but it is _involved_. If you are not familiar with building complex packages from scratch, including modifying Perl scripts, it is advisable that you find an appropriate pre-build.
>
> You aren't exactly going for FIPS compliance, here. And if that doesn't mean anything to you, then you _really_ don't need to go to the trouble.

#### ZLib (optional)

If you have the ZLib development library, Mud98 will have MCCP (MUD Client Compression Protocol) enabled out-of-the-box. This will greatly reduce the amount of traffic needed between the client and server. Even having another package like Strawberry Perl (which I installed to build OpenSSL) is enough for CMake to find a useable copy of ZLib.

### Building on Linux and Cygwin

Building on both Linux and Cygwin is largely the same. In the `src` directory are scripts for both configuring CMake and compiling. 

#### Configuration

Do this to configure CMake to build Mud98:

```sh
$ cd src
$ ./config
```

This will detect your system default C compiler and generate Ninja multi-config build scripts for it. This means that you can create debug and release builds without reconfiguring.

If you have both GCC and Clang, and you want use one over the other, you can tell the config script which one you want:

```sh
$ ./config gcc        # Force GCC
$ ./config clang      # Force Clang
```

This step will tell you missing any required or optional dependencies.

> If downloading from GitHub, you may need to `chmod +x` these scripts before they will execute.

#### Compilation

Once the CMake configuration is complete, you build the Mud98 source like so:

```sh
$ ./compile
```

By default, this creates a debug build in `.build/Debug/`. You can tell the script which build to make like so:

```sh
$ ./compile --config=debug            # Debug
$ ./compile --config=release          # Release
$ ./compile --config=relwithdebinfo   # Release With Debug Info
```

The last option compiles the source with Release optimizations, but leaves source symbols in the binary to aid with debugging. This is a good option if you want to run in "production" but still have the ability to meaningfully use stack traces.

### Building on Windows

If you're an old hand at CMake and/or Powershell, you should have no problem porting the Bash scripts for Linux over to Batch or Powershell. For everyone else, I strongly recommend Visual Studio 19 or higher for its out-of-the-box CMake capability.

> Mud98 has been tested with Visual Studio Community Edition, which is available for free.

#### Using Visual Studio

Open the `src` folder (using `Open Folder...`) in Visual Studio with CMake support enabled. This will automatically configure CMake scripts for the project.

 You may need to manually set the following CMake properties for OpenSSL:
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

#### Using Windows command line

You need to know where `cmake.exe` lives. In my case, for instance, for Visual Studio 2022 Community Edition, it would be here:

```console
> C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe
```
You can then configure and build from the command line:

```console
> cmake.exe -G "Ninja Multi-Config" -B out
> cmake --build out --clean-first --config Debug
```

## Configuring Mud98

Once Mud98 is built, it's ready to configure.

### SSL certificates

In order to accept TLS connections, Mud98 needs an SSL certificate and private key in the `keys` folder.

For convenience during development, there is a Bash script in the `keys` folder called `ssl_gen_keys` that will create self-signed certificates sufficent to get you up and running. **These files are for local development purposes, only.** Many MUD clients and secure terminals will not accept self-signed certificates without an override from the user. Don't make them do this.

### Edit mud98.cfg

In the root folder is a file called `mud98.cfg`. This is where Mud98 allows a number of customizations. The version in the root folder is the "default" version, and is designed to run as, in effect, a "stock ROM". Using the values in this file, you can create a very, very different MUD.

At the very least, you want to edit and verify these lines:

```properties
hostname = localhost

telnet_enabled = yes
telnet_port = 4000

tls_enabled = yes
tls_port = 4043

#keys_dir = keys
#cert_file = mud98.pem
#pkey_file = mud98.key
```

Lines starting with `#` are comments; properties that are commented out display their defaults, if used. In this case, the cert and pkey file will be assumed to be `keys/mud98.pem` and `keys/mud98.key` unless you uncomment these lines and change their values.

Also be sure to check out MSSP settings. You will likely want to change these, also.

## Booting Mud98

Mud98 needs to know where to find `mud98.cfg`. You can run it from the same directory as its config like so:

```sh
$ ./path/to/mud98
```

...or, if your binary is in an arbitrary directory:

```sh
$ ./mud98 -d /path/to/mud98.cfg
```

Either way, `mud98.cfg` needs to be in the root folder, as all paths specified therein are relative paths from that file.

You should see something like this:

```
Wed Oct 11 10:10:13 2023 :: Initializing SSL server:
* Context created.
* Certificate keys/mud98.pem loaded.
* Private key keys/mud98.key loaded.
* Cert/pkey validated.
Wed Oct 11 10:10:13 2023 :: Mud98 is ready to rock on ports 4000 (telnet) & 4043 (tls).
```

At this point Mud98 ready to connect to.

## Creating an implementor

Before you can meaningfully customize (or even administrate) Mud98, you need an "Implementor" character of max level that can log in.

These are the steps to create an initial Implementor:
1. Boot up Mud98.
2. Log in as a new character, and complete character generation. The choices you make don't matter, as you will be able to change any aspect of your own character after leveling.
3. After finishing character creation and getting dumped in the starting area, `QUIT` the session.
4. Open your player file (in the `player` folder) in a text editor.
5. Change your level to 60 and your OLC security to max by finding these lines:
    ```
    Levl 3
    Sec  0
    ```
    ...and changing them to this:
    ```
    Levl 60
    Sec  9
    ```
6. Log back into the game as the new character. Now this character is an Implementor with the highest online editing security setting.

Next: [Worldcrafting from Scratch Pt. 2 &mdash; New Beginnings](wb-02-new-beginnings)

Home: [Documentation](index)
