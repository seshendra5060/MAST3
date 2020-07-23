
#ifndef __mast_parameter_data_h__
#define __mast_parameter_data_h__

// MAST includes
#include <mast/base/mast_data_types.h>
#include <mast/base/exceptions.hpp>


namespace MAST {
namespace Base {

class ParameterData {
  
public:
    
    ParameterData() { }
    
    virtual ~ParameterData() { }
    
    template <typename T>
    T& add(const std::string& nm) {
        
        Error(false, "Parameter type not handled");
        return T();
    }
        
    template <>
    int& add<int>(const std::string& nm) {
        
        return _add_to_map<int>(nm, _int_data);
    }

    template <>
    real_t& add<real_t>(const std::string& nm) {
        
        return _add_to_map<real_t>(nm, _real_data);
    }

    
    template <typename T>
    T get(const std::string& nm) const {
        
        Error(false, "Parameter type not handled");
        return T();
    }
    

    template <>
    int get<int>(const std::string& nm) const {
        
        return _get_from_map<int>(nm, _int_data);
    }
    
    
    template <>
    real_t get<real_t>(const std::string& nm) const {
        
        return _get_from_map<real_t>(nm, _real_data);
    }

private:
    
    template <typename T>
    inline T& _add_to_map(const std::string               &nm,
                          std::map<const std::string, T>  &m) {
        
        typename std::map<const std::string, T>::iterator
        it  = m.find(nm);
        
        Assert0(it == m.end(), "Data already exists for name: " + nm);
        
        m[nm] = T();
        return m[nm];
    }

    
    template <typename T>
    inline T _get_from_map(const std::string                     &nm,
                           const std::map<const std::string, T>  &m) const {
        
        typename std::map<const std::string, T>::const_iterator
        it  = m.find(nm);
        
        Assert0(it != m.end(), "Data does not exist for name: " + nm);
        
        return it->second;
    }

    
    std::map<const std::string, int_t>    _int_data;
    std::map<const std::string, real_t>   _real_data;
};

}  // namespace Base
}  // namespace MAST


#endif // __mast_parameter_data_h__
