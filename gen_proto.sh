#!/bin/sh

cd ./proto && ../_bin/protoc ./*.proto --cpp_out ./
