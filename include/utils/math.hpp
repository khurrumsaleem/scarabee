#ifndef SCARABEE_MATH_H
#define SCARABEE_MATH_H

namespace scarabee {

double exp(double x);

// Evaluates 1 - exp(-x)
double mexp(double x);

double Ki3(double x);
double Ki3_quad(double x);

}  // namespace scarabee

#endif
