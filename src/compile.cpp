//Copyright(C) 2024 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <filesystem>
#include <memory>
#include <array>
#include <iostream>
#include <thread>

#include "compile.hpp"
#include "gui.hpp"
#include "core.hpp"
#include "fileUtils.hpp"

using std::filesystem::exists;
using std::unique_ptr;
using std::array;
using std::cout;
using std::runtime_error;
using std::thread;
using std::filesystem::path;

using Graphics::GUI;
using Core::Compiler;
using Graphics::GUI;
using Utils::File;

namespace Core
{
	void TheCompiler::Compile()
	{
		thread CompileThread([]()
			{
				isCompiling = true;

#ifdef _WIN32
				string hubBuilder = (path(Compiler::projectsPath) / "Elypso-hub" / "build_windows.bat").string();
				string engineBuilder = (path(Compiler::projectsPath) / "Elypso-engine" / "Engine" / "build_windows.bat").string();
				string engineLibraryBuilder = (path(Compiler::projectsPath) / "Elypso-engine" / "Engine library" / "build_windows.bat").string();
#elif __linux__
				string hubBuilder = (path(Compiler::projectsPath) / "Elypso-hub" / "build_linux.sh").string();
				string engineBuilder = (path(Compiler::projectsPath) / "Elypso-engine" / "Engine" / "build_linux.sh").string();
				string engineLibraryBuilder = (path(Compiler::projectsPath) / "Elypso-engine" / "Engine library" / "build_linux.sh").string();
#endif
				string builder;
				if (GUI::target == GUI::Target::Hub) builder = hubBuilder;
				if (GUI::target == GUI::Target::Engine)
				{
					builder = !finishedEngineBuild
						? engineBuilder
						: engineLibraryBuilder;
				}

				string command = "";

				switch (compileType)
				{
				case CompileType::clean_rebuild:
				{
#ifdef _WIN32
					command = "cmd /c \"" + builder + "\" cmake clang skipwait";
#elif __linux__
					command = "bash \"" + builder + "\" cmake skipwait";
#endif
					break;
				}
				case CompileType::compile:
				{
#ifdef _WIN32
					command = "cmd /c \"" + builder + "\" build clang skipwait";
#elif __linux__
					command = "bash \"" + builder + "\" build skipwait";
#endif
					break;
				}
				}

				//command to run the batch file and capture errors
				string fullCommand = command + " 2>&1"; //redirect stderr to stdout

				array<char, 128> buffer{};
#ifdef _WIN32
				unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(fullCommand.c_str(), "r"), _pclose);
#elif __linux__
				auto pipe = unique_ptr<FILE, void(*)(FILE*)>(
					popen(fullCommand.c_str(), "r"),
					[](FILE* file) { if (file) pclose(file); }
				);
#endif
				if (!pipe)
				{
					throw runtime_error("_popen() failed!");
				}

				//read the output line by line and add to the provided vector
				while (fgets(
					buffer.data(),
					static_cast<int>(buffer.size()),
					pipe.get()) != nullptr)
				{
					GUI::output.emplace_back(buffer.data());
				}

				if (!finishedEngineBuild
					&& !finishedLibraryBuild)
				{
					finishedEngineBuild = true;
				}
				else if (finishedEngineBuild
						 && !finishedLibraryBuild)
				{
					finishedLibraryBuild = true;
					CopyLibraryAfterCompile();
				}

				GUI::FinishCompile();
			});

		CompileThread.detach();
	}

	void TheCompiler::CopyLibraryAfterCompile()
	{
		string engineLibraryRootFolder = (path(Compiler::projectsPath) / "Elypso-engine" / "Engine library").string();
		string engineRootFolder = (path(Compiler::projectsPath) / "Elypso-engine" / "Engine").string();

#ifdef _WIN32
		string originLibPath;
		string targetLibPath;

		if (clangCompile)
		{
#ifdef _DEBUG
			originLibPath = (path(engineLibraryRootFolder) / "out" / "build" / "clang-x64-debug" / "libElypso engine.a").string();
#else
			originLibPath = (path(engineLibraryRootFolder) / "out" / "build" / "clang-x64-release" / "libElypso engine.a").string();
#endif
			targetLibPath = (path(engineRootFolder) / "libElypso engine.a").string();
		}

		else if (msvcCompile)
		{
#ifdef _DEBUG
			originLibPath = (path(engineLibraryRootFolder) / "out" / "build" / "msvc-x64-debug" / "Elypso engine.lib").string();
#else
			originLibPath = (path(engineLibraryRootFolder) / "out" / "build" / "msvc-x64-release" / "Elypso engine.lib").string();
#endif
			targetLibPath = (path(engineRootFolder) / "Elypso engine.lib").string();
		}

		//failed to copy library after compile
		if (!exists(originLibPath))
		{
			cout << "origin lib path: " << originLibPath << "\n";
			Compiler::CreateErrorPopup("Elypso engine library failed to compile!");
		}

		File::CopyFileOrFolder(originLibPath, targetLibPath);
#elif __linux__
		string originLibPath = (path(engineLibraryRootFolder) / "out" / "build" / "x64-release" / "libElypso engine.a").string();
		string targetLibPath = (path(engineRootFolder) / "libElypso engine.a").string();
		File::CopyFileOrFolder(originLibPath, targetLibPath);
#endif
	}
}
