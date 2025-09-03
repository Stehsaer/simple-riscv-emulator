#pragma once

#include <iostream>
#include <print>

#include <chrono>

template <typename... Args>
void iprintln(std::format_string<Args...> __fmt, Args&&... __args)
{
	std::println(
		std::cerr,
		"\033[1m\033[36m[INFO]\033[0m {}",
		std::format(__fmt, std::forward<Args>(__args)...)
	);
}

template <typename... Args>
void wprintln(std::format_string<Args...> __fmt, Args&&... __args)
{
	std::println(
		std::cerr,
		"\033[1m\033[33m[WARN]\033[0m {}",
		std::format(__fmt, std::forward<Args>(__args)...)
	);
}

template <typename... Args>
void eprintln(std::format_string<Args...> __fmt, Args&&... __args)
{
	std::println(
		std::cerr,
		"\033[1m\033[91m[ERROR]\033[0m {}",
		std::format(__fmt, std::forward<Args>(__args)...)
	);
}