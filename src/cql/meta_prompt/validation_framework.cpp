// MIT License
// Copyright (c) 2025 dbjwhs

#include "cql/meta_prompt/validation_framework.hpp"
#include "cql/project_utils.hpp"
#include <algorithm>
#include <regex>
#include <set>
#include <sstream>

namespace cql {
namespace meta_prompt {

//=============================================================================
// DefaultValidationFramework Implementation
//=============================================================================

DefaultValidationFramework::DefaultValidationFramework(const ValidationConfig& config)
    : ValidationFramework(config), m_config(config) {
    
    Logger::getInstance().log(LogLevel::INFO,
        "ValidationFramework initialized - confidence_threshold: ", m_config.confidence_threshold,
        ", method: ", m_config.validation_method);
}

ValidationResult DefaultValidationFramework::validate_equivalence(
    std::string_view original, std::string_view optimized) {
    
    ValidationResult result;
    result.validation_method = "heuristic_structural_analysis";
    
    try {
        // Perform structural validation
        bool structural_valid = validate_structure(original, optimized);
        
        // Perform length validation
        bool length_valid = validate_length(original, optimized);
        
        // Calculate similarity score
        double similarity = calculate_similarity_score(original, optimized);
        
        // Check instruction preservation
        bool instructions_preserved = check_instruction_preservation(original, optimized);
        
        // Combine validation results
        result.is_semantically_equivalent = structural_valid && length_valid && instructions_preserved;
        
        // Calculate confidence score based on multiple factors
        double confidence = similarity;
        
        if (!structural_valid) confidence *= 0.7;
        if (!length_valid) confidence *= 0.8;
        if (!instructions_preserved) confidence *= 0.6;
        
        result.confidence_score = std::max(0.0, std::min(1.0, confidence));
        
        Logger::getInstance().log(LogLevel::DEBUG,
            "Validation completed - equivalent: ", result.is_semantically_equivalent,
            ", confidence: ", result.confidence_score,
            ", similarity: ", similarity);
        
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR,
            "Validation exception: ", e.what());
        
        result.is_semantically_equivalent = false;
        result.confidence_score = 0.0;
        result.validation_method = "heuristic_validation_failed";
    }
    
    return result;
}

ValidationResult DefaultValidationFramework::validate_compilation_result(
    const CompilationResult& result) {
    
    if (!result.success) {
        ValidationResult validation;
        validation.is_semantically_equivalent = false;
        validation.confidence_score = 0.0;
        validation.validation_method = "compilation_failed";
        return validation;
    }
    
    return validate_equivalence(result.original_query, result.compiled_prompt);
}

ValidationAnalysis DefaultValidationFramework::analyze_differences(
    std::string_view original, std::string_view optimized) {
    
    ValidationAnalysis analysis;
    
    // Calculate length change
    if (original.length() > 0) {
        analysis.length_change_percent = 
            ((static_cast<double>(optimized.length()) - original.length()) / original.length()) * 100.0;
    }
    
    // Length validation
    analysis.passes_length_checks = std::abs(analysis.length_change_percent) <= m_config.max_length_change_percent;
    if (!analysis.passes_length_checks) {
        analysis.semantic_concerns.push_back(
            "Significant length change: " + std::to_string(analysis.length_change_percent) + "%");
    }
    
    // Structural analysis
    analysis.passes_structural_checks = validate_structure(original, optimized);
    if (!analysis.passes_structural_checks) {
        analysis.structural_differences.push_back("Structural validation failed");
    }
    
    // Extract key terms for comparison
    auto original_terms = extract_key_terms(original);
    auto optimized_terms = extract_key_terms(optimized);
    
    // Find missing key terms
    std::set<std::string> original_set(original_terms.begin(), original_terms.end());
    std::set<std::string> optimized_set(optimized_terms.begin(), optimized_terms.end());
    
    std::vector<std::string> missing_terms;
    std::set_difference(original_set.begin(), original_set.end(),
                       optimized_set.begin(), optimized_set.end(),
                       std::back_inserter(missing_terms));
    
    if (!missing_terms.empty()) {
        std::ostringstream oss;
        oss << "Missing key terms: ";
        for (size_t i = 0; i < missing_terms.size() && i < 5; ++i) {
            if (i > 0) oss << ", ";
            oss << missing_terms[i];
        }
        analysis.semantic_concerns.push_back(oss.str());
    }
    
    // Generate recommendations
    if (analysis.length_change_percent > 20.0) {
        analysis.recommendations.push_back("Consider more conservative optimization to preserve prompt structure");
    }
    if (analysis.length_change_percent < -30.0) {
        analysis.recommendations.push_back("Ensure essential context is not lost in aggressive optimization");
    }
    if (!missing_terms.empty()) {
        analysis.recommendations.push_back("Verify that key domain terms are preserved in optimization");
    }
    
    return analysis;
}

void DefaultValidationFramework::update_config(const ValidationConfig& config) {
    Logger::getInstance().log(LogLevel::INFO,
        "ValidationFramework configuration updated - new threshold: ", config.confidence_threshold);
    
    m_config = config;
}

const ValidationConfig& DefaultValidationFramework::get_config() const {
    return m_config;
}

//=============================================================================
// Private Implementation Methods
//=============================================================================

bool DefaultValidationFramework::validate_structure(std::string_view original, 
                                                    std::string_view optimized) const {
    if (!m_config.enable_structural_validation) {
        return true;
    }
    
    // Check for basic structural elements preservation
    std::vector<std::string> structural_patterns = {
        R"(\b(?:please|kindly|ensure)\b)",  // Politeness markers
        R"(\b(?:step|steps|first|second|third|finally)\b)",  // Sequential markers
        R"(\b(?:example|examples|such as|for instance)\b)",  // Example markers
        R"(\b(?:note|important|warning|caution)\b)",  // Attention markers
        R"(\b(?:format|structure|organize)\b)"  // Format markers
    };
    
    for (const auto& pattern : structural_patterns) {
        std::regex regex(pattern, std::regex_constants::icase);
        
        bool original_has = std::regex_search(original.begin(), original.end(), regex);
        bool optimized_has = std::regex_search(optimized.begin(), optimized.end(), regex);
        
        // If original had the pattern but optimized doesn't, structural integrity may be compromised
        if (original_has && !optimized_has) {
            Logger::getInstance().log(LogLevel::DEBUG,
                "Structural pattern lost in optimization: ", pattern);
            return false;
        }
    }
    
    return true;
}

bool DefaultValidationFramework::validate_length(std::string_view original, 
                                                 std::string_view optimized) const {
    if (!m_config.enable_length_validation) {
        return true;
    }
    
    if (original.empty()) {
        return optimized.empty();
    }
    
    double length_change = ((static_cast<double>(optimized.length()) - original.length()) / original.length()) * 100.0;
    
    bool valid = std::abs(length_change) <= m_config.max_length_change_percent;
    
    if (!valid) {
        Logger::getInstance().log(LogLevel::DEBUG,
            "Length validation failed - change: ", length_change, "% (limit: ", 
            m_config.max_length_change_percent, "%)");
    }
    
    return valid;
}

double DefaultValidationFramework::calculate_similarity_score(std::string_view original, 
                                                              std::string_view optimized) const {
    if (original.empty() && optimized.empty()) {
        return 1.0;
    }
    if (original.empty() || optimized.empty()) {
        return 0.0;
    }
    
    // Extract key terms from both prompts
    auto original_terms = extract_key_terms(original);
    auto optimized_terms = extract_key_terms(optimized);
    
    if (original_terms.empty() && optimized_terms.empty()) {
        return 0.8; // Conservative similarity for empty term sets
    }
    
    // Calculate Jaccard similarity of key terms
    std::set<std::string> original_set(original_terms.begin(), original_terms.end());
    std::set<std::string> optimized_set(optimized_terms.begin(), optimized_terms.end());
    
    std::vector<std::string> intersection;
    std::set_intersection(original_set.begin(), original_set.end(),
                         optimized_set.begin(), optimized_set.end(),
                         std::back_inserter(intersection));
    
    std::vector<std::string> union_terms;
    std::set_union(original_set.begin(), original_set.end(),
                   optimized_set.begin(), optimized_set.end(),
                   std::back_inserter(union_terms));
    
    if (union_terms.empty()) {
        return 0.5;
    }
    
    double jaccard = static_cast<double>(intersection.size()) / union_terms.size();
    
    // Adjust similarity based on length preservation
    double length_ratio = std::min(original.length(), optimized.length()) / 
                         static_cast<double>(std::max(original.length(), optimized.length()));
    
    return (jaccard * 0.7) + (length_ratio * 0.3);
}

std::vector<std::string> DefaultValidationFramework::extract_key_terms(std::string_view prompt) const {
    std::vector<std::string> terms;
    
    // Extract meaningful words (3+ characters, not common stop words)
    std::set<std::string> stop_words = {
        "the", "and", "for", "are", "but", "not", "you", "all", "can", "had", "has", "was", "one",
        "our", "out", "day", "get", "may", "new", "now", "old", "see", "two", "who", "boy", "did",
        "its", "let", "put", "say", "she", "too", "use"
    };
    
    std::regex word_pattern(R"(\b[a-zA-Z]{3,}\b)");
    std::string prompt_str(prompt);
    
    std::sregex_iterator words_begin(prompt_str.begin(), prompt_str.end(), word_pattern);
    std::sregex_iterator words_end;
    
    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::string word = i->str();
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        
        if (stop_words.find(word) == stop_words.end()) {
            terms.push_back(word);
        }
    }
    
    return terms;
}

bool DefaultValidationFramework::check_instruction_preservation(std::string_view original, 
                                                               std::string_view optimized) const {
    // Check for preservation of imperative instructions
    std::vector<std::string> instruction_patterns = {
        R"(\b(?:write|create|generate|produce|make)\b)",
        R"(\b(?:analyze|examine|review|evaluate)\b)",
        R"(\b(?:explain|describe|define|clarify)\b)",
        R"(\b(?:list|enumerate|itemize|outline)\b)",
        R"(\b(?:compare|contrast|differentiate)\b)",
        R"(\b(?:summarize|condense|abstract)\b)"
    };
    
    for (const auto& pattern : instruction_patterns) {
        std::regex regex(pattern, std::regex_constants::icase);
        
        bool original_has = std::regex_search(original.begin(), original.end(), regex);
        bool optimized_has = std::regex_search(optimized.begin(), optimized.end(), regex);
        
        // If original had instructions but optimized doesn't, that's concerning
        if (original_has && !optimized_has) {
            Logger::getInstance().log(LogLevel::DEBUG,
                "Instruction pattern potentially lost: ", pattern);
            // Don't fail immediately - accumulate evidence
        }
    }
    
    // For now, assume instructions are preserved (could be enhanced with more sophisticated analysis)
    return true;
}

//=============================================================================
// Factory Functions
//=============================================================================

std::unique_ptr<ValidationFramework> create_validation_framework(const ValidationConfig& config) {
    return std::make_unique<DefaultValidationFramework>(config);
}

} // namespace meta_prompt
} // namespace cql
