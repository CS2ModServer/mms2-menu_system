# Menu System
# Copyright (C) 2024-2025 komashchenko & Wend4r
# Licensed under the GPLv3 license. See LICENSE file in the project root for details.

if(NOT ENTITY_MANAGER_DIR)
	message(FATAL_ERROR "ENTITY_MANAGER_DIR is empty")
endif()

set(ENTITY_MANAGER_INCLUDE_DIRS
	${ENTITY_MANAGER_INCLUDE_DIRS}

	${ENTITY_MANAGER_DIR}/public
)
