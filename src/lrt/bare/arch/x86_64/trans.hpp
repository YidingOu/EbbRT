/*
  EbbRT: Distributed, Elastic, Runtime
  Copyright (C) 2013 SESA Group, Boston University

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef EBBRT_LRT_TRANS_HPP
#error "Don't include this file directly'"
#endif

namespace ebbrt {
  namespace lrt {
    namespace trans {
      constexpr uintptr_t LOCAL_MEM_VIRT = 0xFFFFFFFF00000000;
      constexpr void* LOCAL_MEM_VIRT_PTR =
        reinterpret_cast<void*>(LOCAL_MEM_VIRT);
    }
  }
}
