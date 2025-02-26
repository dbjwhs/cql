// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../../headers/project_utils.hpp"

// Define static members for the Logger
std::weak_ptr<Logger> Logger::m_instance;
std::mutex Logger::m_instance_mutex;