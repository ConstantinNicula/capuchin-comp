#!/bin/bash
valgrind -s --tool=memcheck --leak-check=full --track-origins=yes $1