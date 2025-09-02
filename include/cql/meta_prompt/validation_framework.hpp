// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>

/**
 * @file validation_framework.hpp
 * @brief Comprehensive validation framework for meta-prompt compilation results
 * 
 * This file provides validation mechanisms to ensure that optimized prompts
 * maintain semantic equivalence with their originals, providing confidence
 * scores and detailed analysis of transformations.
 */

namespace cql {
namespace meta_prompt {

/**
 * @brief Configuration for validation behavior
 */
struct ValidationConfig {
    double confidence_threshold = 0.85;        ///< Minimum confidence for acceptance
    bool enable_llm_validation = true;         ///< Use LLM-based semantic validation
    bool enable_structural_validation = true;  ///< Check structural preservation
    bool enable_length_validation = true;      ///< Validate reasonable length changes
    double max_length_change_percent = 50.0;   ///< Maximum acceptable length change
    std::string validation_method = "hybrid";   ///< Validation approach
};

/**
 * @brief Detailed validation analysis
 */
struct ValidationAnalysis {
    std::vector<std::string> structural_differences; ///< Structural changes detected
    std::vector<std::string> semantic_concerns;      ///< Potential semantic issues
    std::vector<std::string> recommendations;        ///< Improvement suggestions
    double length_change_percent = 0.0;              ///< Percentage length change
    bool passes_structural_checks = true;            ///< Structural validation result
    bool passes_length_checks = true;                ///< Length validation result
};

/**
 * @brief Validation framework for meta-prompt compilation
 * 
 * ValidationFramework provides comprehensive validation of meta-prompt
 * compilation results to ensure semantic equivalence and quality.
 * It combines multiple validation approaches for robust analysis.
 */
class ValidationFramework {
public:
    /**
     * @brief Constructor with configuration
     * 
     * @param config ValidationConfig with validation settings
     */
    explicit ValidationFramework(const ValidationConfig& /* config */ = {}) {}

    /**
     * @brief Destructor
     */
    virtual ~ValidationFramework() = default;

    /**
     * @brief Validate semantic equivalence between original and optimized prompts
     * 
     * Performs comprehensive validation using multiple techniques to assess
     * whether the optimized prompt maintains semantic equivalence with the original.
     * 
     * @param original Original prompt text
     * @param optimized Optimized prompt text
     * @return ValidationResult with confidence score and analysis
     */
    [[nodiscard]] virtual ValidationResult validate_equivalence(
        std::string_view original, std::string_view optimized) = 0;

    /**
     * @brief Validate a compilation result for quality and correctness
     * 
     * @param result CompilationResult to validate
     * @return ValidationResult with detailed analysis
     */
    [[nodiscard]] virtual ValidationResult validate_compilation_result(
        const CompilationResult& result) = 0;

    /**
     * @brief Get detailed analysis of validation results
     * 
     * @param original Original prompt
     * @param optimized Optimized prompt
     * @return ValidationAnalysis with detailed breakdown
     */
    [[nodiscard]] virtual ValidationAnalysis analyze_differences(
        std::string_view original, std::string_view optimized) = 0;

    /**
     * @brief Check if validation framework is available
     * 
     * @return true if validation can be performed
     */
    [[nodiscard]] virtual bool is_available() const = 0;

    /**
     * @brief Update configuration
     * 
     * @param config New validation configuration
     */
    virtual void update_config(const ValidationConfig& config) = 0;

    /**
     * @brief Get current configuration
     * 
     * @return Current ValidationConfig
     */
    [[nodiscard]] virtual const ValidationConfig& get_config() const = 0;
};

/**
 * @brief Default implementation of ValidationFramework
 * 
 * Provides heuristic-based validation with optional LLM enhancement.
 * Uses structural analysis, length checks, and pattern matching.
 */
class DefaultValidationFramework : public ValidationFramework {
public:
    /**
     * @brief Constructor with configuration
     */
    explicit DefaultValidationFramework(const ValidationConfig& config = {});

    /**
     * @brief Destructor
     */
    ~DefaultValidationFramework() override = default;

    // ValidationFramework interface
    [[nodiscard]] ValidationResult validate_equivalence(
        std::string_view original, std::string_view optimized) override;

    [[nodiscard]] ValidationResult validate_compilation_result(
        const CompilationResult& result) override;

    [[nodiscard]] ValidationAnalysis analyze_differences(
        std::string_view original, std::string_view optimized) override;

    [[nodiscard]] bool is_available() const override { return true; }

    void update_config(const ValidationConfig& config) override;

    [[nodiscard]] const ValidationConfig& get_config() const override;

private:
    /**
     * @brief Perform structural validation
     */
    [[nodiscard]] bool validate_structure(std::string_view original, 
                                          std::string_view optimized) const;

    /**
     * @brief Perform length validation
     */
    [[nodiscard]] bool validate_length(std::string_view original, 
                                       std::string_view optimized) const;

    /**
     * @brief Calculate semantic similarity score
     */
    [[nodiscard]] double calculate_similarity_score(std::string_view original, 
                                                     std::string_view optimized) const;

    /**
     * @brief Extract key terms from prompt
     */
    [[nodiscard]] std::vector<std::string> extract_key_terms(std::string_view prompt) const;

    /**
     * @brief Check for critical instruction preservation
     */
    [[nodiscard]] bool check_instruction_preservation(std::string_view original, 
                                                      std::string_view optimized) const;

    // Configuration
    ValidationConfig m_config;
};

/**
 * @brief Factory function to create validation framework
 * 
 * @param config Optional configuration
 * @return Unique pointer to ValidationFramework implementation
 */
[[nodiscard]] std::unique_ptr<ValidationFramework> create_validation_framework(
    const ValidationConfig& config = {});

} // namespace meta_prompt
} // namespace cql
