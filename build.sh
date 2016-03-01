#!/bin/bash
clang++ -stdlib=libc++ -std=c++14 example.cpp -lboost_coroutine -lboost_context -lboost_system -lboost_thread -pthread
