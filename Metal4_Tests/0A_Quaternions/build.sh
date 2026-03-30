#!/bin/sh

set -e

odin build src -out:build/out -debug -strict-style -vet -warnings-as-errors -disallow-do
