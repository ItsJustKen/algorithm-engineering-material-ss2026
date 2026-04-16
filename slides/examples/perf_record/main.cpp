// Particle simulation with a deliberate O(n²) hotspot.
//
// Purpose: a simple program to demonstrate `perf record` / `perf report`.
// The compute_forces() function dominates runtime and shows up clearly
// in perf's output, making it easy to see where optimization effort belongs.
//
// Build:  g++ -O2 -g -o particle_sim main.cpp -lm
// Run:    ./particle_sim
// Profile:
//   perf record -g ./particle_sim
//   perf report

#include <cmath>
#include <cstdio>
#include <vector>

struct Particle {
  double x, y;
  double vx, vy;
  double fx, fy;
};

// O(n²) all-pairs force computation — this should dominate the profile.
void compute_forces(std::vector<Particle> &particles) {
  const double softening = 1e-9;
  for (auto &p : particles) {
    p.fx = 0.0;
    p.fy = 0.0;
  }
  for (size_t i = 0; i < particles.size(); ++i) {
    for (size_t j = i + 1; j < particles.size(); ++j) {
      double dx = particles[j].x - particles[i].x;
      double dy = particles[j].y - particles[i].y;
      double dist = std::sqrt(dx * dx + dy * dy + softening);
      double force = 1.0 / (dist * dist);
      double fx = force * dx / dist;
      double fy = force * dy / dist;
      particles[i].fx += fx;
      particles[i].fy += fy;
      particles[j].fx -= fx;
      particles[j].fy -= fy;
    }
  }
}

// Simple Euler integration — fast, should be a small fraction of runtime.
void update_positions(std::vector<Particle> &particles, double dt) {
  for (auto &p : particles) {
    p.vx += p.fx * dt;
    p.vy += p.fy * dt;
    p.x += p.vx * dt;
    p.y += p.vy * dt;
  }
}

void simulate(std::vector<Particle> &particles, int steps, double dt) {
  for (int s = 0; s < steps; ++s) {
    compute_forces(particles);
    update_positions(particles, dt);
  }
}

int main() {
  const int n = 4000;
  const int steps = 10;
  const double dt = 0.001;

  std::vector<Particle> particles(n);
  // Deterministic initialization.
  for (int i = 0; i < n; ++i) {
    double angle = 2.0 * M_PI * i / n;
    double r = 1.0 + 0.5 * (i % 7);
    particles[i] = {r * std::cos(angle), r * std::sin(angle), 0, 0, 0, 0};
  }

  simulate(particles, steps, dt);

  // Prevent dead-code elimination.
  double checksum = 0;
  for (const auto &p : particles) {
    checksum += p.x + p.y;
  }
  std::printf("checksum: %.6f\n", checksum);
  return 0;
}
