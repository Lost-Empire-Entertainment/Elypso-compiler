//Copyright(C) 2024 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <iostream>
#include <filesystem>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "glfw3.h"
#include "magic_enum.hpp"

#include "gui.hpp"
#include "render.hpp"
#include "configFile.hpp"
#include "core.hpp"
#include "compile.hpp"
#include "fileUtils.hpp"

using std::cout;
using std::filesystem::directory_iterator;
using std::filesystem::path;

using Core::ConfigFile;
using Core::Compiler;
using Core::TheCompiler;
using Utils::File;

namespace Graphics
{
	static ImVec4 bgrColor;

	void GUI::GUIInitialize()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::SetCurrentContext(ImGui::GetCurrentContext());
		ImGuiIO& io = ImGui::GetIO();

		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		static string tempString = (path(Compiler::docsPath) / "imgui.ini").string();
		const char* customConfigPath = tempString.c_str();
		io.IniFilename = customConfigPath;

		ImGui_ImplGlfw_InitForOpenGL(Render::window, true);
		ImGui_ImplOpenGL3_Init("#version 330");

		io.Fonts->Clear();
		io.Fonts->AddFontFromFileTTF(((path(Compiler::filesPath) / "fonts" / "coda" / "Coda-Regular.ttf").string()).c_str(), 16.0f);

		bgrColor.x = Render::backgroundColor.x;
		bgrColor.y = Render::backgroundColor.y;
		bgrColor.z = Render::backgroundColor.z;
		bgrColor.w = 1.0f;

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		io.FontGlobalScale = stof(ConfigFile::GetValue("fontScale"));

		isImguiInitialized = true;
	}

	int GUI::GetScreenWidth()
	{
		int width, height;
		glfwGetFramebufferSize(Render::window, &width, &height);
		return width;
	}

	int GUI::GetScreenHeight()
	{
		int width, height;
		glfwGetFramebufferSize(Render::window, &width, &height);
		return height;
	}

	void GUI::GUILoop()
	{
		if (isImguiInitialized)
		{
			if (!Compiler::IsUserIdle())
			{
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplGlfw_NewFrame();
				ImGui::NewFrame();

				ImGuiDockNodeFlags dockFlags =
					ImGuiDockNodeFlags_PassthruCentralNode;

				ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), dockFlags);

				RenderParentWindow();

				ImGui::Render();
			}

			ImDrawData* drawData = ImGui::GetDrawData();
			if (drawData != nullptr)
			{
				ImGui_ImplOpenGL3_RenderDrawData(drawData);
			}
		}
	}

	void GUI::RenderParentWindow()
	{
		int width = GetScreenWidth();
		int height = GetScreenHeight();
		ImGui::SetNextWindowSize(ImVec2(
			static_cast<float>(width),
			static_cast<float>(height)),
			ImGuiCond_Always);
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);

		ImGuiWindowFlags windowFlags =
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoSavedSettings;

		if (ImGui::Begin("##Main", NULL, windowFlags))
		{
			RenderWindowContent();
		}
	}

	void GUI::RenderWindowContent()
	{
		static string progressText = "Waiting for input...";

		if (!isBuilding)
		{
			ImVec2 windowSize = ImGui::GetWindowSize();

			ImVec2 topButtonSize(120, 50);

			ImVec2 topButton1Pos(
				(windowSize.x * 0.4f) - (topButtonSize.x * 0.5f), 15.0f);
			ImVec2 topButton2Pos(
				(windowSize.x * 0.6f) - (topButtonSize.x * 0.5f), 15.0f);

			ImVec2 checkbox_clang(
				(windowSize.x * 0.75f) - (topButtonSize.x * 0.5f), 15.0f);
			ImVec2 checkbox_msvc(
				(windowSize.x * 0.79f) - (topButtonSize.x * 0.5f), 15.0f);

			ImVec2 checkbox_release(
				(windowSize.x * 0.84f) - (topButtonSize.x * 0.5f), 15.0f);
			ImVec2 checkbox_debug(
				(windowSize.x * 0.88f) - (topButtonSize.x * 0.5f), 15.0f);

			ImVec2 topSideButton(
				(windowSize.x * 0.95f) - (topButtonSize.x * 0.5f), 25.0f);

			//press hub button to switch to the hub choices
			ImGui::SetCursorPos(topButton1Pos);
			if (ImGui::Button("Hub", topButtonSize))
			{
				if (target != Target::Hub)
				{
					string windowTitle = "Compiler | Elypso hub";
					glfwSetWindowTitle(Render::window, windowTitle.c_str());

					output.clear();

					string msg = "---- Switched to Elypso hub compilation.";
					cout << msg << "\n";
					output.emplace_back(msg);
					progressText = "Waiting for input...";

					target = Target::Hub;
				}
			}

			//press engine button to switch to the engine choices
			ImGui::SetCursorPos(topButton2Pos);
			if (ImGui::Button("Engine", topButtonSize))
			{
				if (target != Target::Engine)
				{
					string windowTitle = "Compiler | Elypso engine";
					glfwSetWindowTitle(Render::window, windowTitle.c_str());

					output.clear();

					string msg = "---- Switched to Elypso engine compilation.";
					cout << msg << "\n";
					output.emplace_back(msg);
					progressText = "Waiting for input...";

					target = Target::Engine;
				}
			}

			//toggle clang compilation
			ImGui::SetCursorPos(checkbox_clang);
			if (ImGui::Checkbox("##compile_clang", &TheCompiler::clangCompile))
			{
				if (TheCompiler::clangCompile)
				{
					string msg = "---- Enabled CLang compilation.";
					cout << msg << "\n";
					output.emplace_back(msg);
				}
				else
				{
					string msg = "---- Disabled CLang compilation.";
					cout << msg << "\n";
					output.emplace_back(msg);
				}
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Text("Toggle CLang compilation.");
				ImGui::EndTooltip();
			}

			//toggle msvc compilation
			ImGui::SetCursorPos(checkbox_msvc);
			if (ImGui::Checkbox("##compile_msvc", &TheCompiler::msvcCompile))
			{
				if (TheCompiler::msvcCompile)
				{
					string msg = "---- Enabled MSVC compilation.";
					cout << msg << "\n";
					output.emplace_back(msg);
				}
				else
				{
					string msg = "---- Disabled MSVC compilation.";
					cout << msg << "\n";
					output.emplace_back(msg);
				}
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Text("Toggle MSVC compilation.");
				ImGui::EndTooltip();
			}

			//toggle release mode build
			ImGui::SetCursorPos(checkbox_release);
			if (ImGui::Checkbox("##compile_release", &TheCompiler::releaseCompile))
			{
				if (TheCompiler::releaseCompile)
				{
					string msg = "---- Enabled Release mode compilation.";
					cout << msg << "\n";
					output.emplace_back(msg);
				}
				else
				{
					string msg = "---- Disabled Release mode compilation.";
					cout << msg << "\n";
					output.emplace_back(msg);
				}
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Text("Toggle Release mode compilation.");
				ImGui::EndTooltip();
			}

			//toggle debug mode compilation
			ImGui::SetCursorPos(checkbox_debug);
			if (ImGui::Checkbox("##compile_debug", &TheCompiler::debugCompile))
			{
				if (TheCompiler::debugCompile)
				{
					string msg = "---- Enabled Debug mode compilation.";
					cout << msg << "\n";
					output.emplace_back(msg);
				}
				else
				{
					string msg = "---- Disabled Debug mode compilation.";
					cout << msg << "\n";
					output.emplace_back(msg);
				}
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Text("Toggle Debug mode compilation.");
				ImGui::EndTooltip();
			}

			//press set path button to assign the folder path of hub/engine
			ImGui::SetCursorPos(topSideButton);
			ImVec2 smallButtonSize = ImVec2(topButtonSize.x * 0.7f, topButtonSize.y * 0.7f);
			if (ImGui::Button("Set path", smallButtonSize))
			{
				string setPath = File::SetPath();

				bool isValidFolder = false;
				for (const auto& folder : directory_iterator(setPath))
				{
					string folderName = path(folder).stem().string();
					if (folderName == "Elypso-engine")
					{
						for (const auto& childFolder : directory_iterator(folder))
						{
							string childfolderName = path(childFolder).filename().string();
							if (childfolderName == "Engine")
							{
								isValidFolder = true;
								break;
							}
						}
						if (isValidFolder) break;
					}
				}
				if (!isValidFolder)
				{
					string msg = "---- Cannot set path to '" + setPath + "' because the chosen folder is not used by Elypso engine! Look for the folder where the Elypso-engine folder is inside of.";

					cout << msg << "\n";
					output.emplace_back(msg);
				}
				else
				{
					for (const auto& entry : directory_iterator(setPath))
					{
						if (path(entry).filename().string() == "Engine")
						{
							isValidFolder = true;
							break;
						}
					}

					if (!isValidFolder)
					{
						string targetName;
						if (target == Target::Hub) targetName = "Elypso hub";
						if (target == Target::Engine) targetName = "Elypso engine";

						string msg = "---- Cannot set path to '" + setPath + "' for "
							+ targetName + " because the chosen folder is not valid!";

						cout << msg << "\n";
						output.emplace_back(msg);
					}
					else
					{
						ConfigFile::SetValue("projectsPath", setPath);

						string msg = "---- Set projects folder path to '" + setPath + "'.";

						cout << msg << "\n";
						output.emplace_back(msg);
					}
				}
			}
		}

		ImGui::SetCursorPosY(75.0f);

		if (!sentMsg)
		{
			if (!isBuilding
				&& !hasBuiltOnce)
			{
				output.emplace_back("---- Switched to Elypso hub compilation.");
			}
			else if (isBuilding)
			{
				string targetName;
				if (target == Target::Hub) targetName = "Elypso hub";
				if (target == Target::Engine) targetName = "Elypso engine";

				string actionName = action == Action::compile
					? "compiling"
					: "clean rebuilding";
				progressText = "Started " + actionName + " " + targetName;
			}
			else if (!isBuilding
				&& hasBuiltOnce)
			{
				progressText = "Waiting for input...";
			}
			sentMsg = true;
		}
		ImGui::Text("%s", progressText.c_str());

		ImVec2 scrollingRegionSize(
			ImGui::GetContentRegionAvail().x,
			ImGui::GetContentRegionAvail().y - 75);
		if (ImGui::BeginChild("ScrollingRegion", scrollingRegionSize, true))
		{
			float wrapWidth = ImGui::GetContentRegionAvail().x - 10;
			ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrapWidth);

			//display the content of the text buffer
			for (const auto& message : output)
			{
				ImGui::TextWrapped("%s", message.c_str());

				/*
				* 
				* DISABLED COPYING TEXT FROM CONSOLE
				* WILL BE RE-ENABLED IN A FUTURE VERSION
				* IF I BOTHER TO FIND A FIX TO CRASH IN VISUAL STUDIO
				* WHEN DOUBLE-CLICKING THE TEXT USER WANTS TO COPY
				* 
					if (ImGui::IsItemClicked()
					&& ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					cout << "---- Added '" << message << "' to clipboard.\n";
					ImGui::SetClipboardText(message.c_str());
				}
				*/
			}

			ImGui::PopTextWrapPos();

			//scrolls to the bottom if scrolling is allowed
			//and if user is close to the newest compilation message
			bool isNearBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10.0f;
			if (isNearBottom
				|| (!firstScrollToBottom))
			{
				ImGui::SetScrollHereY(1.0f);
				firstScrollToBottom = true;
			}
		}

		ImGui::EndChild();

		if (!isBuilding)
		{
			ImVec2 windowSize = ImGui::GetWindowSize();

			ImVec2 buttonSize(120, 50);

			ImVec2 button1Pos(
				(windowSize.x * 0.3f) - (buttonSize.x * 0.5f),
				windowSize.y - buttonSize.y - 15.0f);
			ImVec2 button2Pos(
				(windowSize.x * 0.5f) - (buttonSize.x * 0.5f),
				windowSize.y - buttonSize.y - 15.0f);
			ImVec2 button3Pos(
				(windowSize.x * 0.7f) - (buttonSize.x * 0.5f),
				windowSize.y - buttonSize.y - 15.0f);

			//press clean rebuild to fully rebuild hub/engine from scratch
			ImGui::SetCursorPos(button1Pos);
			if (ImGui::Button("Clean rebuild", buttonSize))
			{
				bool canCleanRebuild = true;

				action = Action::clean_rebuild;

				string targetName = target == Target::Hub ? "Elypso hub" : "Elypso engine";

				if (!TheCompiler::clangCompile
					&& !TheCompiler::msvcCompile)
				{
					string msg = "---- Cannot clean rebuild if neither clang or msvc have been selected!";
					cout << msg << "\n";
					output.emplace_back(msg);

					canCleanRebuild = false;
				}
				if (TheCompiler::clangCompile
					&& TheCompiler::msvcCompile)
				{
					string msg = "---- Cannot clean rebuild if both clang and msvc have been selected!";
					cout << msg << "\n";
					output.emplace_back(msg);

					canCleanRebuild = false;
				}
				if (!TheCompiler::releaseCompile
					&& !TheCompiler::debugCompile)
				{
					string msg = "---- Cannot clean rebuild if neither release or debug have been selected!";
					cout << msg << "\n";
					output.emplace_back(msg);

					canCleanRebuild = false;
				}
				if (TheCompiler::releaseCompile
					&& TheCompiler::debugCompile)
				{
					string msg = "---- Cannot clean rebuild if both release and debug have been selected!";
					cout << msg << "\n";
					output.emplace_back(msg);

					canCleanRebuild = false;
				}

				if (Compiler::projectsPath == "")
				{
					string msg = "---- Cannot clean rebuild " + targetName + " because the projects folder has not been assigned yet!";

					cout << msg << "\n";
					output.emplace_back(msg);

					canCleanRebuild = false;
				}

				if (target == Target::Hub
					&& Compiler::IsThisProcessAlreadyRunning("Elypso hub.exe"))
				{
					string msg = "---- Cannot clean rebuild " + targetName + " because Elypso hub is running!";

					cout << msg << "\n";
					output.emplace_back(msg);

					canCleanRebuild = false;
				}
				if (target == Target::Engine
					&& Compiler::IsThisProcessAlreadyRunning("Elypso engine.exe"))
				{
					string msg = "---- Cannot clean rebuild " + targetName + " because Elypso engine is running!";

					cout << msg << "\n";
					output.emplace_back(msg);

					canCleanRebuild = false;
				}

				if (canCleanRebuild)
				{
					output.clear();

					string msg = "---- Started clean rebuilding " + targetName;

					cout << msg << "\n";
					output.emplace_back(msg);

					if (!hasBuiltOnce) hasBuiltOnce = true;
					isBuilding = true;
					if (sentMsg) sentMsg = false;

					TheCompiler::compileType = TheCompiler::CompileType::clean_rebuild;

					if (TheCompiler::clangCompile
						&& !TheCompiler::msvcCompile)
					{
						targetCompiler = TargetCompiler::clang;
					}
					else if (!TheCompiler::clangCompile
						&& TheCompiler::msvcCompile)
					{
						targetCompiler = TargetCompiler::msvc;
					}

					if (TheCompiler::releaseCompile
						&& !TheCompiler::debugCompile)
					{
						targetVersion = TargetVersion::release;
					}
					else if (!TheCompiler::releaseCompile
						&& TheCompiler::debugCompile)
					{
						targetVersion = TargetVersion::debug;
					}

					if (target == Target::Engine
						&& (TheCompiler::finishedEngineBuild
						|| TheCompiler::finishedLibraryBuild))
					{
						TheCompiler::finishedEngineBuild = false;
						TheCompiler::finishedLibraryBuild = false;
					}

					if (GUI::target == GUI::Target::Hub)
					{
						TheCompiler::finishedEngineBuild = false;
						TheCompiler::finishedLibraryBuild = false;
					}

					TheCompiler::Compile();
				}
			}

			//press compile button to start compiling hub/engine
			ImGui::SetCursorPos(button2Pos);
			if (ImGui::Button("Compile", buttonSize))
			{
				bool canCompile = true;

				action = Action::compile;

				string targetName = target == Target::Hub ? "Elypso hub" : "Elypso engine";

				if (!TheCompiler::clangCompile
					&& !TheCompiler::msvcCompile)
				{
					string msg = "---- Cannot compile if neither clang or msvc have been selected!";
					cout << msg << "\n";
					output.emplace_back(msg);

					canCompile = false;
				}
				if (TheCompiler::clangCompile
					&& TheCompiler::msvcCompile)
				{
					string msg = "---- Cannot compile if both clang and msvc have been selected!";
					cout << msg << "\n";
					output.emplace_back(msg);

					canCompile = false;
				}
				if (!TheCompiler::releaseCompile
					&& !TheCompiler::debugCompile)
				{
					string msg = "---- Cannot compile if neither release or debug have been selected!";
					cout << msg << "\n";
					output.emplace_back(msg);

					canCompile = false;
				}
				if (TheCompiler::releaseCompile
					&& TheCompiler::debugCompile)
				{
					string msg = "---- Cannot compile if both release and debug have been selected!";
					cout << msg << "\n";
					output.emplace_back(msg);

					canCompile = false;
				}

				if (Compiler::projectsPath == "")
				{
					string msg = "---- Cannot compile " + targetName + " because the projects folder has not been assigned yet!";

					cout << msg << "\n";
					output.emplace_back(msg);

					canCompile = false;
				}

				if (target == Target::Hub
					&& Compiler::IsThisProcessAlreadyRunning("Elypso hub.exe"))
				{
					string msg = "---- Cannot compile " + targetName + " because Elypso hub is running!";

					cout << msg << "\n";
					output.emplace_back(msg);

					canCompile = false;
				}
				if (target == Target::Engine
					&& Compiler::IsThisProcessAlreadyRunning("Elypso engine.exe"))
				{
					string msg = "---- Cannot compile " + targetName + " because Elypso engine is running!";

					cout << msg << "\n";
					output.emplace_back(msg);

					canCompile = false;
				}

				if (Compiler::projectsPath == "")
				{
					string msg = "---- Cannot compile " + targetName + " because the projects folder has not been assigned yet!";

					cout << msg << "\n";
					output.emplace_back(msg);

					canCompile = false;
				}

				if (target == Target::Hub
					&& Compiler::IsThisProcessAlreadyRunning("Elypso hub.exe"))
				{
					string msg = "---- Cannot compile " + targetName + " because Elypso hub is running!";

					cout << msg << "\n";
					output.emplace_back(msg);

					canCompile = false;
				}
				if (target == Target::Engine
					&& Compiler::IsThisProcessAlreadyRunning("Elypso engine.exe"))
				{
					string msg = "---- Cannot compile " + targetName + " because Elypso engine is running!";

					cout << msg << "\n";
					output.emplace_back(msg);

					canCompile = false;
				}

				if (canCompile)
				{
					output.clear();

					string msg = "---- Started compiling " + targetName;

					cout << msg << "\n";
					output.emplace_back(msg);

					if (!hasBuiltOnce) hasBuiltOnce = true;
					isBuilding = true;
					if (sentMsg) sentMsg = false;

					TheCompiler::compileType = TheCompiler::CompileType::compile;

					if (TheCompiler::clangCompile
						&& !TheCompiler::msvcCompile)
					{
						targetCompiler = TargetCompiler::clang;
					}
					else if (!TheCompiler::clangCompile
						&& TheCompiler::msvcCompile)
					{
						targetCompiler = TargetCompiler::msvc;
					}

					if (TheCompiler::releaseCompile
						&& !TheCompiler::debugCompile)
					{
						targetVersion = TargetVersion::release;
					}
					else if (!TheCompiler::releaseCompile
						&& TheCompiler::debugCompile)
					{
						targetVersion = TargetVersion::debug;
					}

					if (target == Target::Engine
						&& (TheCompiler::finishedEngineBuild
							|| TheCompiler::finishedLibraryBuild))
					{
						TheCompiler::finishedEngineBuild = false;
						TheCompiler::finishedLibraryBuild = false;
					}

					if (GUI::target == GUI::Target::Hub)
					{
						TheCompiler::finishedEngineBuild = false;
						TheCompiler::finishedLibraryBuild = false;
					}

					TheCompiler::Compile();
				}
			}

			//press clear output to clean all messages from output window
			ImGui::SetCursorPos(button3Pos);
			if (ImGui::Button("Clear output", buttonSize))
			{
				output.clear();

				progressText = "Waiting for input...";
			}
		}

		ImGui::End();
	}

	void GUI::FinishCompile()
	{
		if (target == Target::Engine
			&& !TheCompiler::finishedLibraryBuild)
		{
			if (TheCompiler::finishedEngineBuild
				&& !TheCompiler::finishedLibraryBuild)
			{
				string msg = "---- Finished compiling Elypso engine";

				cout << msg << "\n";
				output.emplace_back(msg);

				msg = "\n--------------------\n\n";

				cout << msg;
				output.emplace_back(msg);

				msg = "---- Started compiling Elypso engine library\n";

				cout << msg;
				output.emplace_back(msg);

				TheCompiler::Compile();
			}
		}
		else
		{
			string targetName = target == Target::Hub ? "Elypso hub" : "Elypso engine";

			switch (action)
			{
			case Action::compile:
			{
				isBuilding = false;
				if (sentMsg) sentMsg = false;

				string msg = "---- Finished compiling " + targetName;

				cout << msg << "\n";
				output.emplace_back(msg);

				Compiler::UpdateLastEndOfCompilerOnceTime();

				break;
			}
			case Action::clean_rebuild:
			{
				isBuilding = false;
				if (sentMsg) sentMsg = false;

				string msg = "---- Finished clean rebuilding " + targetName;

				cout << msg << "\n";
				output.emplace_back(msg);

				Compiler::UpdateLastEndOfCompilerOnceTime();

				break;
			}
			}
		}
	}

	void GUI::GUIShutdown()
	{
		if (isImguiInitialized)
		{
			isImguiInitialized = false;

			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
		}
	}
}