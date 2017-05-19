/**
 * Copyright (C) Metaswitch Networks
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/

#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <atomic>
#include "oidtree.hpp"

extern OIDTree tree;
extern std::atomic_long last_seen_time;

#endif /** BONOLATENCYTABLE_H */
