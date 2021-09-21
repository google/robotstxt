
# Google Robots.txt Parser and Matcher Library

The repository contains Google's robots.txt parser and matcher as a C++ library
(compliant to C++11).

## About the library

The Robots Exclusion Protocol (REP) is a standard that enables website owners to
control which URLs may be accessed by automated clients (i.e. crawlers) through
a simple text file with a specific syntax. It's one of the basic building blocks
of the internet as we know it and what allows search engines to operate.

Because the REP was only a de-facto standard for the past 25 years, different
implementers implement parsing of robots.txt slightly differently, leading to
confusion. This project aims to fix that by releasing the parser that Google
uses.

The library is slightly modified (i.e. some internal headers and equivalent
symbols) production code used by Googlebot, Google's crawler, to determine which
URLs it may access based on rules provided by webmasters in robots.txt files.
The library is released open-source to help developers build tools that better
reflect Google's robots.txt parsing and matching.

For webmasters, we included a small binary in the project that allows testing a
single URL and user-agent against a robots.txt.

## Building the library

### Quickstart

We included with the library a small binary to test a local robots.txt against a
user-agent and URL. Running the included binary requires:

*   A compatible platform (e.g. Windows, macOS, Linux, etc.). Most platforms are
    fully supported.
*   A compatible C++ compiler supporting at least C++11. Most major compilers
    are supported.
*   [Git](https://git-scm.com/) for interacting with the source code repository.
    To install Git, consult the
    [Set Up Git](https://help.github.com/articles/set-up-git/) guide on
    [GitHub](https://github.com/).
*   Although you are free to use your own build system, most of the
    documentation within this guide will assume you are using
    [Bazel](https://bazel.build/). To download and install Bazel (and any of its
    dependencies), consult the
    [Bazel Installation Guide](https://docs.bazel.build/versions/master/install.html)

#### Building with Bazel

[Bazel](https://bazel.build/) is the official build system for the library,
which is supported on most major platforms (Linux, Windows, MacOS, for example)
and compilers.

To build and run the binary:

```bash
$ git clone https://github.com/google/robotstxt.git robotstxt
Cloning into 'robotstxt'...
...
$ cd robotstxt/
bazel-robots$ bazel test :robots_test
...
/:robots_test                                                      PASSED in 0.1s

Executed 1 out of 1 test: 1 test passes.
...
bazel-robots$ bazel build :robots_main
...
Target //:robots_main up-to-date:
  bazel-bin/robots_main
...
bazel-robots$ bazel run robots_main -- ~/local/path/to/robots.txt YourBot https://example.com/url
  user-agent 'YourBot' with url 'https://example.com/url' allowed: YES
```

#### Building with CMake

[CMake](https://cmake.org) is the community-supported build system for the
library.

To build the library using CMake, just follow the steps below:

```bash
$ git clone https://github.com/google/robotstxt.git robotstxt
Cloning into 'robotstxt'...
...
$ cd robotstxt/
...
$ mkdir c-build && cd c-build
...
$ cmake .. -DROBOTS_BUILD_TESTS=ON
...
$ make
...
$ make test
Running tests...
Test project robotstxt/c-build
    Start 1: robots-test
1/1 Test #1: robots-test ......................   Passed    0.02 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.02 sec
...
$ robots ~/local/path/to/robots.txt YourBot https://example.com/url
  user-agent 'YourBot' with url 'https://example.com/url' allowed: YES
```

## Notes

Parsing of robots.txt files themselves is done exactly as in the production
version of Googlebot, including how percent codes and unicode characters in
patterns are handled. The user must ensure however that the URI passed to the
AllowedByRobots and OneAgentAllowedByRobots functions, or to the URI parameter
of the robots tool, follows the format specified by RFC3986, since this library
will not perform full normalization of those URI parameters. Only if the URI is
in this format, the matching will be done according to the REP specification.

Also note that the library, and the included binary, do not handle
implementation logic that a crawler might apply outside of parsing and matching,
for example: `Googlebot-Image` respecting the rules specified for `User-agent:
Googlebot` if not explicitly defined in the robots.txt file being tested.

## License

The robots.txt parser and matcher C++ library is licensed under the terms of the
Apache license. See LICENSE for more information.

## Links

To learn more about this project:

*   check out the
    [internet draft](https://tools.ietf.org/html/draft-koster-rep),
*   how
    [Google's handling robots.txt](https://developers.google.com/search/reference/robots_txt),
*   or for a high level overview, the
    [robots.txt page on Wikipedia](https://en.wikipedia.org/wiki/Robots_exclusion_standard).
