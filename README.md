# MQLib

MQLib is a C++ library that provides a Pub-Sub framework for Component decoupling. It also includes different utilities as:

- ```List```: Lightweight implementation of double linked list functionalities

- ```Heap```: Portable implementation of HEAP management (alloc, free)

---
---

## Changelog

---
### **28 Jan 2019*
  
- [x] Modified behaviour with multiple "any" wildcard. Now subscriptions to topics like: ```stat/+/+/+``` require 4 level tokens to be satisfied. For example, publishing on ```stat/var/1``` don't be handled as it has only 3 levels (stat, var, 1). On the other hand, publishing on ```stat/param/var/1``` will be properly handled as it has 4 levels and perfectly matches subscription format ```stat/+/+/+```
- [x] New release tagged as v1.1.0  

---
### **17 Jan 2019*
  
- [x] Full update sync (no changes added)

---
### **05 Mar 2018**
  
- [x] Bug handling ```WildcardScope``` solved

---
### **16 Apr 2018**
  
- [x] Updated wildcard ```WildcardScope``` by  ```WildcardScopeDev```, ```WildcardScopeGroup```
- [x] Updated ```AddrField``` (prev = 2, now = 1)

  
