#pragma once
#include <string>

enum reg {
	rax, rbx, rcx, rdx, r8, r9
};

enum size {
	i8, i16, i32, i64
};

inline std::string _suffix(size s) {
	switch (s) {
	case i8: return "b";
	case i16: return "w";
	case i32: return "l";
	case i64: return "q";
	}
}

inline std::string _register(reg r, size s) {
	switch (r) {
	case rax:
		switch (s) {
		case i8: return "al";
		case i16: return "ax";
		case i32: return "eax";
		case i64: return "rax";
		}
	case rbx:
		switch (s) {
		case i8: return "bl";
		case i16: return "bx";
		case i32: return "ebx";
		case i64: return "rbx";
		}
	case rcx:
		switch (s) {
		case i8: return "cl";
		case i16: return "cx";
		case i32: return "ecx";
		case i64: return "rcx";
		}
	case rdx:
		switch (s) {
		case i8: return "dl";
		case i16: return "dx";
		case i32: return "edx";
		case i64: return "rdx";
		}
	case r8:
		switch (s) {
		case i8: return "r8b";
		case i16: return "r8w";
		case i32: return "r8d";
		case i64: return "r8";
		}
	case r9:
		switch (s) {
		case i8: return "r9b";
		case i16: return "r9w";
		case i32: return "r9d";
		case i64: return "r9";
		}
	}
	return "";
}