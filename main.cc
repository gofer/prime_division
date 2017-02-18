#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <thread>
#include <mutex>
#include <primes.h>

typedef unsigned long long int ulli;
const ulli MAX_ULLI = 0xFFFFFFFFFFFFFFFF;
const ulli MIN_ULLI = 0x0000000000000000;

const ulli CACHE_SIZE = 1e+6;
const ulli BUFFERING_LINES = 1e+6;

struct Factor {
  ulli prime, exp;
  Factor(ulli prime, ulli exp) : prime(prime), exp(exp) {}
  bool operator==(const Factor &f) const { return prime == f.prime; }
  bool operator< (const Factor &f) const { return prime <  f.prime; }
};

std::ostream& operator<<(std::ostream &os, const Factor &f)
{
  os << '{' << f.prime << ", " << f.exp << '}';
  return os;
}

typedef std::set<Factor> FactorSet;

std::ostream& operator<<(std::ostream &os, const FactorSet &s)
{
  auto itr = s.begin();
  while(itr != s.end()) {
    os << *itr;
    ++itr;
    if(itr != s.end()) os << ", ";
  }
  return os;
}

std::map<ulli, FactorSet> cache;
std::mutex cache_insert_mutex;

FactorSet prime_division(ulli n) {
  FactorSet s;
  if (n == 1) return s;
  
  auto itr = cache.find(n);
  if(itr != cache.end()) {
    return itr->second;
  }
  
  for (ulli i = 0; i < primes.size(); ++i) {
    if (n < primes[i]) break;
    
    if (n % primes[i] == 0) {
      s = prime_division(n / primes[i]);
      auto itr = s.find(Factor(primes[i], 0));
      if (itr != s.end()) {
        Factor f = *itr;
        s.erase(itr);
        ++f.exp;
        s.insert(f);
      } else {
        s.emplace(primes[i], 1);
      }
      break;
    }
  }
  
  if (n <= CACHE_SIZE) {
    cache_insert_mutex.lock();
    cache.emplace(n, s);
    cache_insert_mutex.unlock();
  }
  
  return s;
}

std::mutex file_mutex;
void output_file(unsigned id, const std::string &s)
{
  std::stringstream fss;
  fss << "result" << "/" << "output" << id << ".txt";
  
  file_mutex.lock();
  std::ofstream ofs(fss.str(), std::ofstream::out | std::ofstream::app);
  ofs << s;
  ofs.close();
  file_mutex.unlock();
}

std::mutex output_mutex;
void debug_display(unsigned id, ulli min, ulli max, ulli n)
{
  ulli NUMBER_OF_FLUSH = ((max - min) / BUFFERING_LINES);
  
  output_mutex.lock();
  std::cerr 
    << "\033[" << ('1' + id) << "E" << "\033[0K"
    << "string buffer output. (thread_id = " << id << ", count = " << ((n - min) / BUFFERING_LINES) << " / " << NUMBER_OF_FLUSH << ")"
    << "\033[" << ('2' + id) << "F"
    << std::endl;
  std::cerr.flush();
  output_mutex.unlock();
}

void thread_func(unsigned id, ulli min, ulli max)
{
  ulli NUMBER_OF_FLUSH = ((max - min) / BUFFERING_LINES);
  debug_display(id, min, max, min);
  
  std::stringstream ss;
  for (ulli n = min; n < max; ++n) {
    ss << n << '\t' << prime_division(n) << std::endl;
    
    if ((n + 1 - min) % BUFFERING_LINES == 0) {
      output_file(id + 1, ss.str());
      ss.str("");
      
      debug_display(id, min, max, n);
    }
  }
  
  debug_display(id, min, max, max);
  
  output_file(id + 1, ss.str());
}

int main(int argc, char **argv)
{
  const ulli N = 1e+9 + 4, K = 8;
  
  std::stringstream ss;
  for (ulli n = 2; n < CACHE_SIZE; ++n) {
    ss << n << '\t' << prime_division(n) << std::endl;
  }
  output_file(0, ss.str());
  
  std::cerr << "finish building cache. (cache size = " << CACHE_SIZE << ")";
  for(unsigned k = 0; k < K; ++k) std::cerr << std::endl;
  std::cerr.flush();
  
  std::vector<std::thread> threads;
  ulli width = (N - CACHE_SIZE) / K;
  for (unsigned k = 0; k < K; ++k) {
    ulli offset = CACHE_SIZE + width * k;
    threads.emplace_back(thread_func, k, offset, offset + width);
  }
  
  for (unsigned k = 0; k < K; ++k) {
    threads[k].join();
  }
  
  std::cerr << "\033[" << ('1' + K) << "E" << std::endl;
  
  return 0;
}
