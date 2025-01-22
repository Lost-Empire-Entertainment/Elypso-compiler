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
					string buildType = TheCompiler::releaseCompile ? "release" : "debug";
					command = "cmd /c \"" + builder + "\" cmake " + buildType + " skipwait";
#elif __linux__
					command = "bash \"" + builder + "\" cmake skipwait";
#endif
					break;
				}
				case CompileType::compile:
				{
#ifdef _WIN32
					string buildType = TheCompiler::releaseCompile ? "release" : "debug";
					command = "cmd /c \"" + builder + "\" build " + buildType + " skipwait";
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
		string targetFolderBuild = GUI::targetVersion == GUI::TargetVersion::release ? "release" : "debug";
		string fullTargetFolderName = "x64-" + targetFolderBuild;
		string libName = targetFolderBuild == "release" ? "Elypso engine.lib" : "Elypso engineD.lib";

		string originLibPath = (path(engineLibraryRootFolder) / "out" / "build" / fullTargetFolderName / libName).string();
		string targetLibPath = (path(engineRootFolder) / libName).string();

		//failed to copy library after compile
		if (!exists(originLibPath))
		{
			cout << "origin lib path: " << originLibPath << "\n";
			Compiler::CreateErrorPopup("Engine library lib failed to compile!");
		}

		File::CopyFileOrFolder(originLibPath, targetLibPath);
#elif __linux__
		string targetFolderBuild = GUI::targetVersion == GUI::TargetVersion::release ? "release" : "debug";
		string libName = targetFolderBuild == "release" ? "libElypso engine.a" : "libElypso engineD.a";

		string originLibPath = (path(engineLibraryRootFolder) / "out" / "build" / "x64-release" / libName).string();
		string targetLibPath = (path(engineRootFolder) / libName).string();

		File::CopyFileOrFolder(originLibPath, targetLibPath);
#endif
	}
}
