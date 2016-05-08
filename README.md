# NewtMQ
[![Build Status](https://travis-ci.org/newtmq/newtmq-server.svg?branch=master)](https://travis-ci.org/newtmq/newtmq-server)
[![MIT License](http://img.shields.io/badge/license-MIT-blue.svg?style=flat)](LICENSE)

NewtMQ is a yet another MOM (Message Oriented Middleware) with STOMP protocol.
This aims to be high speed response and perform high-throughput. This project will try various measures to obtain these features.

# Getting Started

## Preparation
You may need to install library that NewtMQ dependents.
In the Debian / Ubuntu distribution, you can install it as below.

```$ sudo apt-get install libconfuse-dev libcunit1-dev```

## Download
You can get the latest released version of NewtMQ from [here](https://github.com/newtmq/newtmq-server/releases).

## Installation
NewtMQ is written in C-language, you may install as below just like some same language products.
```$ ./configure
$ make
$ sudo make install```

When you want to build latest development version from GitHub sources. You have to install `autoconf` to make build environment as below.
```$ autoreconf -i```

## Starting Server
You can start NewtMQ by `newtd` command.

```
$ newtd
```

NewtMQ listens on port 61613 by default. You can change it to edit configuraton file. Here is an example of it.

```
server = "127.0.0.1"
port = 12345
loglevel = "DEBUG"
```

When you save this as `newd.conf`, NewtMQ can load it by '-c, --config' parameter. This is usage of NewtMQ.
```
Usage: newtd [OPTION...]

  -c, --config=config_path   filepath of configuration for newtd
  -?, --help                 Give this help list
      --usage                Give a short usage message
```

If you want to stop NewtMQ, please input 'Ctrll-C' then it will be stopped.
