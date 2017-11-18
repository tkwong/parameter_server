#include <iostream>
#include <iomanip>

#include <limits>
#include <vector>
#include <numeric>
#include <algorithm>
#include <chrono>
#include <cmath>

#include <thread>
#include <future>


template <typename TimeT = std::chrono::microseconds>
class Benchmark
{
public:
    Benchmark(int num_iterations=1, int throw_away=0)
        : m_num_iter(num_iterations),
          m_throw_away(throw_away),
          m_times(),
          m_mean(),
          m_st_dev()
    { }
          
    template <typename Fun, typename... Args>
    std::vector< typename std::result_of< Fun(Args...) >::type >
    benchmark(Fun&& fun, Args&&... args)
    {
        using result_t = typename std::result_of< Fun(Args...) >::type;

        std::vector< result_t > results;
        m_times.clear();
        auto n = m_num_iter + m_throw_away;

        for (auto i = 0; i != n; i++)
        {
            auto pair = measure(std::forward<Fun>(fun),
                                          std::forward<Args>(args)...);
            m_times.push_back(pair.first);
            results.push_back(pair.second);
        }

        compute_mean();
        compute_st_dev();

        return results;
    }

    void reset()
    {
      m_mean = 0.;
      m_st_dev = 0.;
      m_times.clear();
    }

    typename TimeT::rep mean() const
    {
        return m_mean;
    }

    typename TimeT::rep standard_deviation() const
    {
        return m_st_dev;
    }

    template <typename Fun, typename... Args>
    static std::pair< typename TimeT::rep,  typename std::result_of< Fun(Args...) >::type >
    measure_with_result(Fun&& fun, Args&&... args)
    {
        auto t1 = std::chrono::steady_clock::now();
        auto result = std::forward<Fun>(fun)(std::forward<Args>(args)...);
        auto t2 = std::chrono::steady_clock::now();

        auto time = std::chrono::duration_cast<TimeT>( t2 - t1 ).count();
        return std::make_pair(time, result);
    }
    
    template <typename Fun, typename... Args>
    static typename TimeT::rep
    measure(Fun&& fun, Args&&... args)
    {
        auto t1 = std::chrono::steady_clock::now();
        std::forward<Fun>(fun)(std::forward<Args>(args)...);
        auto t2 = std::chrono::steady_clock::now();

        return std::chrono::duration_cast<TimeT>( t2 - t1 ).count();
    }
    
private:
    void compute_mean()
    {
        auto sum = std::accumulate(m_times.begin() + m_throw_away, m_times.end(), 0);
        m_mean = sum / m_num_iter;
    }

    void compute_st_dev()
    {
        std::vector<typename TimeT::rep> diff(m_num_iter);
        std::transform(m_times.begin() + m_throw_away, m_times.end(), diff.begin(),
                       [this](typename TimeT::rep t) {return t - this->m_mean;});

        auto sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0);
        m_st_dev = std::sqrt(sq_sum / m_num_iter);
    }

    int m_num_iter;

    int m_throw_away;

    std::vector<typename TimeT::rep> m_times;

    typename TimeT::rep m_mean;

    typename TimeT::rep m_st_dev;
};
