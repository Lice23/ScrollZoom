#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#pragma warning(push)
#include "F4SE/F4SE.h"
#include "RE/Fallout.h"

#ifdef NDEBUG
#include <spdlog/sinks/basic_file_sink.h>
#else
#include <spdlog/sinks/msvc_sink.h>
#endif
#include <ShlObj.h>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <utility>
#pragma warning(pop)

#include "SimpleIni.h"

#define DLLEXPORT __declspec(dllexport)

namespace logger
{
template <class... Args> void trace(spdlog::format_string_t<Args...> a_fmt, Args &&...a_args)
{
    spdlog::trace(a_fmt, std::forward<Args>(a_args)...);
}

template <class... Args> void debug(spdlog::format_string_t<Args...> a_fmt, Args &&...a_args)
{
    spdlog::debug(a_fmt, std::forward<Args>(a_args)...);
}

template <class... Args> void info(spdlog::format_string_t<Args...> a_fmt, Args &&...a_args)
{
    spdlog::info(a_fmt, std::forward<Args>(a_args)...);
}

template <class... Args> void warn(spdlog::format_string_t<Args...> a_fmt, Args &&...a_args)
{
    spdlog::warn(a_fmt, std::forward<Args>(a_args)...);
}

template <class... Args> void error(spdlog::format_string_t<Args...> a_fmt, Args &&...a_args)
{
    spdlog::error(a_fmt, std::forward<Args>(a_args)...);
}

template <class... Args> void critical(spdlog::format_string_t<Args...> a_fmt, Args &&...a_args)
{
    spdlog::critical(a_fmt, std::forward<Args>(a_args)...);
}

[[nodiscard]] inline std::optional<std::filesystem::path> log_directory()
{
    wchar_t *buffer{nullptr};
    const auto result =
        SHGetKnownFolderPath(FOLDERID_Documents, KNOWN_FOLDER_FLAG::KF_FLAG_DEFAULT, nullptr, std::addressof(buffer));
    std::unique_ptr<wchar_t[], decltype(&CoTaskMemFree)> knownPath(buffer, CoTaskMemFree);
    if (!knownPath || result != S_OK)
    {
        return std::nullopt;
    }

    std::filesystem::path path = knownPath.get();
    path /= L"My Games/Fallout4/F4SE";
    return path;
}
} // namespace logger

using namespace std::literals;

#include "Version.h"
