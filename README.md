# DsmrParserLite
C++11 parser for the DSMR V5 protocol.<br>
The parser is built using [re2c](https://re2c.org/) tool.

## Features
* Header only
* No dynamic memory allocation
* High performance
* Can be used on bare metal embedded systems

## Limitations
* Only supports DSMR V5
* Doesn't parse multiline fields like `1-0:99.97.0(3)(...)(...)(...)...`. Such fields are ignored

## How to use
* Include the header file in your project
* Follow the [usage example](https://github.com/PolarGoose/DsmrParserLite/blob/main/src/Test/DsmrParser/Example.cpp) that shows how to use this library

## References
* [DSMR 5.0.2 P1 Companion Standard](https://www.netbeheernederland.nl/publicatie/dsmr-502-p1-companion-standard)
