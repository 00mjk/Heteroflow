#pragma once

#include "../facility/facility.hpp"

namespace hf {

// Class: Node
class Node {

  friend class TaskBase;
  friend class HostTask;
  friend class PullTask;
  friend class PushTask;
  friend class KernelTask;
  friend class FlowBuilder;
  
  // Host data
  struct Host {

    Host() = default;

    template <typename C>
    Host(C&&);

    std::function<void()> work;
  };
  
  // Pull data
  struct Pull {

    Pull() = default;

    template <typename T>
    Pull(const T*, size_t);

    ~Pull();

    int device {0};
    const void* h_data {nullptr};
    void*  d_data {nullptr};
    size_t h_size {0};
    size_t d_size {0};
  };
  
  // Push data
  struct Push {

    Push() = default;

    template <typename T>
    Push(T*, Node*, size_t);

    void* h_data {nullptr};
    Node* source {nullptr};
    size_t h_size {0};
  };
  
  // Kernel data
  struct Kernel {

    Kernel() = default;

    template <typename F, typename... ArgsT>
    Kernel(F&&, ArgsT&&...);

    int device {0};
    ::dim3 grid;
    ::dim3 block;
    size_t shm_size {0};
    cudaStream_t stream;
  };

  public:

    template <typename... ArgsT>
    Node(ArgsT&&...);
    
    bool is_host() const;
    bool is_push() const;
    bool is_pull() const;
    bool is_kernel() const;

  private:
    
    std::string _name;

    nonstd::variant<Host, Pull, Push, Kernel> _handle;

    std::vector<Node*> _successors;
    std::vector<Node*> _dependents;
    
    std::atomic<int> _num_dependents {0};

    void _precede(Node*);
};

// ----------------------------------------------------------------------------
// Host field
// ----------------------------------------------------------------------------

// Constructor
template <typename C>
Node::Host::Host(C&& callable) : 
  work {std::forward<C>(callable)} {
}

// ----------------------------------------------------------------------------
// Pull field
// ----------------------------------------------------------------------------

// Constructor
template <typename T>
Node::Pull::Pull(const T* data, size_t N) : 
  h_data {data},
  h_size {N * sizeof(T)} {
}

// Destructor
inline Node::Pull::~Pull() {
  if(d_data) {
    ::cudaSetDevice(device);
    ::cudaFree(d_data);
  }
}

// ----------------------------------------------------------------------------
// Push field
// ----------------------------------------------------------------------------

// Constructor
template <typename T>
Node::Push::Push(T* tgt, Node* src, size_t N) : 
  h_data {tgt},
  source {src},
  h_size {N * sizeof(T)} {
}
    
// ----------------------------------------------------------------------------
// Kernel field
// ----------------------------------------------------------------------------

template <typename F, typename... ArgsT>
Node::Kernel::Kernel(F&& func, ArgsT&&... args) {

  using Traits = function_traits<F>;
  static_assert(Traits::arity == 2,"");
  static_assert(std::is_same<typename Traits::return_type,void>::value,"");
  static_assert(std::is_same<typename Traits::argument<0>::type, size_t>::value, "");
  //static_assert(std::is_same<Traits::argument<1>::type,int>::value,"");

  // TODO
  func<<<2, 2>>>(std::forward<ArgsT>(args)...);
}

// ----------------------------------------------------------------------------

// Constructor
template <typename... ArgsT>
Node::Node(ArgsT&&... args) : 
  _handle {std::forward<ArgsT>(args)...} {
}

// Procedure: _precede
inline void Node::_precede(Node* rhs) {
  _successors.push_back(rhs);
  rhs->_dependents.push_back(this);
  rhs->_num_dependents.fetch_add(1, std::memory_order_relaxed);
}

// ----------------------------------------------------------------------------


}  // end of namespace hf -----------------------------------------------------





