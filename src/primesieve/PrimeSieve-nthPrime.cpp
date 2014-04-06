///
/// @file  PrimeSieve-nthPrime.cpp
///
/// Copyright (C) 2014 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include <primesieve/config.hpp>
#include <primesieve/PrimeSieve.hpp>
#include <primesieve/SieveOfEratosthenes.hpp>
#include <primesieve/pmath.hpp>
#include <primesieve.hpp>

#include <stdint.h>
#include <algorithm>
#include <cmath>

using namespace primesieve;
using namespace std;

namespace {

void checkLimit(uint64_t start, uint64_t dist)
{
  if (dist > get_max_stop() - start)
    throw primesieve_error("nth prime is too large > 2^64 - 2^32 * 11");
}

void checkLowerLimit(uint64_t stop)
{
  if (stop == 0)
    throw primesieve_error("nth prime < 2 is impossible, negative n is too small");
}

void checkLowerLimit(uint64_t start, double dist)
{
  double s = static_cast<double>(start);
  s = max(1E4, s);
  if (s / dist < 0.9)
    throw primesieve_error("nth prime < 2 is impossible, negative n is too small");
}

int64_t pix(int64_t n)
{
  double x = static_cast<double>(n);
  double logx = log(max(4.0, x));
  double pix = x / logx;

  return static_cast<int64_t>(pix);
}

bool sieveBackwards(int64_t n, int64_t count, uint64_t stop)
{
  return (count >= n) && !(count == n && stop < 2);
}

uint64_t nthPrimeDistance(int64_t n, uint64_t start, int64_t direction = 1)
{
  double x = static_cast<double>(n);
  double s = static_cast<double>(start);

  x = abs(x);
  x = max(4.0, x);
  s = max(4.0, s);

  double logx = log(x);
  double loglogx = log(logx);
  double pix = x * (logx + loglogx - 1);

  // Correct start if sieving backwards
  if (direction < 0)
    s -= pix;

  // Approximate the nth prime using
  // start + n * log(start + pi(n) / log log n))
  double logStartPix = log(max(4.0, s + pix / loglogx));
  double dist = max(pix, x * logStartPix);
  double maxPrimeGap = logStartPix * logStartPix;

  // Make sure start + dist <= nth prime
  dist += sqrt(dist) * log(logStartPix) * 2.0 * -direction;
  dist = max(dist, maxPrimeGap);

  if (direction < 0)
    checkLowerLimit(start, dist);

  return static_cast<uint64_t>(dist);
}

} // namespace

namespace primesieve {

/// This class is used to generate n primes and
/// then stop by throwing an exception.
///
class NthPrime : public Callback<uint64_t> {
public:
  NthPrime() : n_(0), nthPrime_(0) { }
  void findNthPrime(uint64_t, uint64_t, uint64_t);
  void callback(uint64_t);
  uint64_t getNthPrime() const;
private:
  uint64_t n_;
  uint64_t nthPrime_;
};

void NthPrime::findNthPrime(uint64_t n, uint64_t start, uint64_t stop)
{
  n_ = n;
  PrimeSieve ps;
  try {
    ps.callbackPrimes(start, stop, this);
    ps.callbackPrimes(stop + 1, get_max_stop(), this);
    throw primesieve_error("nth prime is too large > 2^64 - 2^32 * 11");
  }
  catch (cancel_callback&) { }
}

uint64_t NthPrime::getNthPrime() const
{
  return nthPrime_;
}

void NthPrime::callback(uint64_t prime)
{
  if (--n_ == 0)
  {
    nthPrime_ = prime;
    throw cancel_callback();
  }
}

uint64_t PrimeSieve::nthPrime(uint64_t n)
{
  return nthPrime(0, n);
}

uint64_t PrimeSieve::nthPrime(int64_t n, uint64_t start)
{
  if (n == 0)
    return 0;

  setStart(start);
  double t1 = getWallTime();

  uint64_t stop = start;
  uint64_t dist = nthPrimeDistance(n, start);
  uint64_t nthPrimeGuess = start + dist;

  int64_t pixSqrtNthPrime = pix(isqrt(nthPrimeGuess));
  int64_t bruteForceMin = 10000;
  int64_t bruteForceThreshold = max(bruteForceMin, pixSqrtNthPrime);
  int64_t count = 0;

  while (sieveBackwards(n, count, stop) || (n - count) > bruteForceThreshold)
  {
    if (count < n)
    {
      dist = nthPrimeDistance(n - count, start);
      checkLimit(start, dist);
      stop = start + dist;
      count += countPrimes(start, stop);
      start = stop + 1;
    }
    if (sieveBackwards(n, count, stop))
    {
      checkLowerLimit(stop);
      dist = nthPrimeDistance(count - n, stop, /* backwards */ -1);
      start = (start > dist) ? start - dist : 1;
      count -= countPrimes(start, stop);
      stop = start - 1;
    }
  }

  if (n < 0) count--;
  dist = nthPrimeDistance(n - count, start) * 2;
  checkLimit(start, dist);
  stop = start + dist;
  NthPrime np;
  np.findNthPrime(n - count, start, stop);
  seconds_ = getWallTime() - t1;

  return np.getNthPrime();
}

} // namespace primesieve
