#!/usr/bin/env python3
"""
Meta-Prompt Compiler Benchmarking Script
Demonstrates optimization performance with detailed metrics
"""

import os
import subprocess
import json
import time
from pathlib import Path
from typing import Dict, List, Tuple

class MetaPromptBenchmark:
    def __init__(self, build_dir: str = None):
        self.demo_dir = Path(__file__).parent
        self.project_root = self.demo_dir.parent
        self.build_dir = build_dir or (self.project_root / "build")
        self.cql_executable = self.build_dir / "cql"
        
        # Verify executable exists
        if not self.cql_executable.exists():
            raise FileNotFoundError(f"CQL executable not found at {self.cql_executable}")
    
    def get_file_stats(self, filepath: Path) -> Dict:
        """Get basic file statistics"""
        if not filepath.exists():
            return {"size": 0, "lines": 0, "chars": 0}
            
        content = filepath.read_text()
        return {
            "size": filepath.stat().st_size,
            "lines": len(content.splitlines()),
            "chars": len(content)
        }
    
    def _sanitize_filename(self, filename: str) -> str:
        """Sanitize filename to prevent path traversal attacks"""
        import re
        # Remove any path separators and dangerous characters
        safe_name = re.sub(r'[^\w\-_.]', '_', filename)
        # Remove leading dots to prevent hidden files
        safe_name = safe_name.lstrip('.')
        # Ensure it's not empty and not too long
        if not safe_name:
            safe_name = "unnamed"
        return safe_name[:50]  # Limit length
    
    def _validate_parameters(self, input_file: Path, mode: str, goal: str, domain: str) -> None:
        """Validate input parameters for security and correctness"""
        # Validate input file
        if not input_file.exists():
            raise ValueError(f"Input file does not exist: {input_file}")
        
        if not input_file.is_file():
            raise ValueError(f"Input path is not a file: {input_file}")
        
        # Validate mode parameter
        valid_modes = ["LOCAL_ONLY", "CACHED_LLM", "FULL_LLM", "ASYNC_LLM"]
        if mode not in valid_modes:
            raise ValueError(f"Invalid mode '{mode}'. Must be one of: {', '.join(valid_modes)}")
        
        # Validate goal parameter
        valid_goals = ["REDUCE_TOKENS", "IMPROVE_ACCURACY", "BALANCED", "DOMAIN_SPECIFIC"]
        if goal not in valid_goals:
            raise ValueError(f"Invalid goal '{goal}'. Must be one of: {', '.join(valid_goals)}")
        
        # Validate domain parameter
        import re
        if not re.match(r'^[a-zA-Z0-9_-]+$', domain):
            raise ValueError(f"Invalid domain '{domain}'. Only alphanumeric characters, underscore, and hyphen allowed")
        
        if len(domain) > 64:
            raise ValueError(f"Domain name too long: {len(domain)} characters (max 64)")

    def run_optimization(self, input_file: Path, mode: str = "LOCAL_ONLY", 
                        goal: str = "BALANCED", domain: str = "software") -> Dict:
        """Run optimization and return metrics"""
        # Validate all input parameters
        try:
            self._validate_parameters(input_file, mode, goal, domain)
        except ValueError as e:
            return {
                "success": False,
                "error": str(e),
                "execution_time": 0.0,
                "mode": mode,
                "goal": goal,
                "domain": domain
            }
        
        # Sanitize filename to prevent path traversal
        safe_filename = self._sanitize_filename(input_file.stem)
        output_file = self.demo_dir / "temp" / f"optimized_{safe_filename}.txt"
        output_file.parent.mkdir(exist_ok=True)
        
        cmd = [
            str(self.cql_executable),
            "--optimize",
            str(input_file),
            str(output_file),
            "--mode", mode,
            "--goal", goal,
            "--domain", domain,
            "--show-metrics",
            "--show-validation"
        ]
        
        start_time = time.time()
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            end_time = time.time()
            
            execution_time = end_time - start_time
            
            # Parse metrics from output (simplified version)
            metrics = {
                "success": result.returncode == 0,
                "execution_time": execution_time,
                "stdout": result.stdout,
                "stderr": result.stderr,
                "mode": mode,
                "goal": goal,
                "domain": domain
            }
            
            # Get output file stats if successful
            if result.returncode == 0 and output_file.exists():
                metrics["output_stats"] = self.get_file_stats(output_file)
            
            return metrics
            
        except subprocess.TimeoutExpired:
            return {
                "success": False,
                "error": "Timeout",
                "execution_time": 30.0,
                "mode": mode,
                "goal": goal,
                "domain": domain
            }
    
    def benchmark_file(self, input_file: Path) -> Dict:
        """Benchmark optimization for a single file"""
        print(f"📊 Benchmarking: {input_file.name}")
        
        original_stats = self.get_file_stats(input_file)
        
        # Test different optimization configurations
        configurations = [
            ("LOCAL_ONLY", "BALANCED", "software"),
            ("LOCAL_ONLY", "REDUCE_TOKENS", "software"),
            ("LOCAL_ONLY", "IMPROVE_ACCURACY", "software"),
            ("LOCAL_ONLY", "DOMAIN_SPECIFIC", "software"),
        ]
        
        results = {
            "file": str(input_file),
            "original_stats": original_stats,
            "optimizations": []
        }
        
        for mode, goal, domain in configurations:
            print(f"  🔄 Testing {mode} + {goal} + {domain}")
            optimization_result = self.run_optimization(input_file, mode, goal, domain)
            results["optimizations"].append(optimization_result)
        
        return results
    
    def generate_report(self, benchmark_results: List[Dict]) -> str:
        """Generate a comprehensive benchmark report"""
        report = ["# Meta-Prompt Compiler Benchmark Report\n"]
        report.append(f"Generated: {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
        report.append("## Executive Summary\n")
        
        total_files = len(benchmark_results)
        successful_optimizations = 0
        total_execution_time = 0.0
        size_reductions = []
        
        for result in benchmark_results:
            for opt in result["optimizations"]:
                if opt["success"]:
                    successful_optimizations += 1
                    total_execution_time += opt["execution_time"]
                    
                    if "output_stats" in opt:
                        original_size = result["original_stats"]["chars"]
                        optimized_size = opt["output_stats"]["chars"]
                        if original_size > 0:
                            reduction = ((original_size - optimized_size) / original_size) * 100
                            size_reductions.append(reduction)
        
        avg_execution_time = total_execution_time / max(successful_optimizations, 1)
        avg_size_reduction = sum(size_reductions) / max(len(size_reductions), 1)
        
        report.append(f"- **Files processed**: {total_files}")
        report.append(f"- **Successful optimizations**: {successful_optimizations}")
        report.append(f"- **Average execution time**: {avg_execution_time:.3f}s")
        report.append(f"- **Average size reduction**: {avg_size_reduction:.1f}%")
        report.append(f"- **Success rate**: {(successful_optimizations / max(total_files * 4, 1)) * 100:.1f}%\n")
        
        # Detailed results
        report.append("## Detailed Results\n")
        
        for result in benchmark_results:
            file_name = Path(result["file"]).name
            original_stats = result["original_stats"]
            
            report.append(f"### {file_name}\n")
            report.append(f"**Original**: {original_stats['chars']} chars, {original_stats['lines']} lines\n")
            
            report.append("| Mode | Goal | Domain | Success | Time (s) | Size Reduction | Status |")
            report.append("|------|------|---------|---------|----------|----------------|--------|")
            
            for opt in result["optimizations"]:
                mode = opt["mode"]
                goal = opt["goal"]
                domain = opt["domain"]
                success = "✅" if opt["success"] else "❌"
                exec_time = f"{opt['execution_time']:.3f}"
                
                if opt["success"] and "output_stats" in opt:
                    original_size = original_stats["chars"]
                    optimized_size = opt["output_stats"]["chars"]
                    reduction = ((original_size - optimized_size) / original_size) * 100 if original_size > 0 else 0
                    size_reduction = f"{reduction:.1f}%"
                    status = "SUCCESS"
                else:
                    size_reduction = "N/A"
                    status = opt.get("error", "FAILED")
                
                report.append(f"| {mode} | {goal} | {domain} | {success} | {exec_time} | {size_reduction} | {status} |")
            
            report.append("")
        
        return "\n".join(report)
    
    def run_full_benchmark(self) -> str:
        """Run complete benchmark suite"""
        print("🚀 Starting Meta-Prompt Compiler Benchmark")
        print("=" * 50)
        
        # Find all example files
        examples_dir = self.demo_dir / "examples"
        if not examples_dir.exists():
            raise FileNotFoundError(f"Examples directory not found: {examples_dir}")
        
        example_files = list(examples_dir.glob("*.llm"))
        if not example_files:
            raise FileNotFoundError(f"No .llm files found in {examples_dir}")
        
        print(f"Found {len(example_files)} example files")
        
        # Run benchmarks
        results = []
        for example_file in example_files:
            result = self.benchmark_file(example_file)
            results.append(result)
        
        # Generate report
        report = self.generate_report(results)
        
        # Save report
        report_file = self.demo_dir / "benchmark_report.md"
        report_file.write_text(report)
        
        print(f"\n📊 Benchmark complete! Report saved to: {report_file}")
        return report

def main():
    """Main entry point"""
    try:
        benchmark = MetaPromptBenchmark()
        report = benchmark.run_full_benchmark()
        print("\n" + "=" * 50)
        print("📈 BENCHMARK SUMMARY")
        print("=" * 50)
        
        # Print executive summary
        lines = report.split('\n')
        in_summary = False
        for line in lines:
            if line.startswith("## Executive Summary"):
                in_summary = True
                continue
            elif line.startswith("##") and in_summary:
                break
            elif in_summary and line.strip():
                print(line)
        
    except Exception as e:
        print(f"❌ Benchmark failed: {e}")
        return 1
    
    return 0

if __name__ == "__main__":
    exit(main())
