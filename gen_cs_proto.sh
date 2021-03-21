#!/bin/sh

cd ./proto && ../unity_client/protogen/protogen -i:game.proto -o:../unity_client/moba_client/Assets/Scripts/netowrk/game.cs
