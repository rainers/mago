#pragma once

#include <stdint.h>
#include <string>
#include <vector>

enum PrintLevel {
	PRINT_NO_VALUES = 0,
	PRINT_ALL_VALUES = 1,
	PRINT_SIMPLE_VALUES = 2,
};

/// MI interface command IDs
enum MiCommandId {
	CMD_UNKNOWN,
	CMD_GDB_EXIT, // -gdb-exit quit
	CMD_HELP, // help
	CMD_EXEC_RUN, // run -exec-run
	CMD_EXEC_CONTINUE, // continue -exec-continue
	CMD_EXEC_INTERRUPT, // interrupt -exec-interrupt
	CMD_EXEC_FINISH, // finish -exec-finish
	CMD_EXEC_NEXT, // next -exec-next
	CMD_EXEC_NEXT_INSTRUCTION, // nexti -exec-next-instruction
	CMD_EXEC_STEP, // step -exec-step
	CMD_EXEC_STEP_INSTRUCTION, // stepi -exec-step-instruction
	CMD_BREAK_INSERT, // break -break-insert
	CMD_BREAK_DELETE, // delete -break-delete
	CMD_BREAK_ENABLE, // enable -break-enable
	CMD_BREAK_DISABLE, // disable -break-disable
	CMD_BREAK_LIST, // -break-list
	CMD_LIST_THREAD_GROUPS, // info break -list-thread-groups
	CMD_THREAD_INFO, // info thread -thread-info
	CMD_THREAD_LIST_IDS, // info threads -thread-list-ids
	CMD_STACK_LIST_FRAMES, // -stack-list-frames backtrace
	CMD_STACK_INFO_DEPTH, // -stack-info-depth
	CMD_STACK_LIST_ARGUMENTS, // -stack-list-variables
	CMD_STACK_LIST_VARIABLES, // -stack-list-variables
	CMD_STACK_LIST_LOCALS, // -stack-list-locals
	CMD_VAR_CREATE, // -var-create
	CMD_VAR_UPDATE, // -var-update
	CMD_VAR_DELETE, // -var-delete
	CMD_VAR_SET_FORMAT, // -var-set-format
	CMD_LIST_FEATURES, // -list-features
	CMD_GDB_VERSION, // -gdb-version
	CMD_ENVIRONMENT_CD, // -environment-cd
	CMD_SET_INFERIOR_TTY, // set inferior-tty
	CMD_GDB_SHOW, // -gdb-show
	CMD_INTERPRETER_EXEC, // -interpreter-exec
	CMD_DATA_EVALUATE_EXPRESSION, // -data-evaluate-expression
	CMD_GDB_SET, // -gdb-set
	CMD_MAINTENANCE, // maintenance
	CMD_ENABLE_PRETTY_PRINTING, // -enable-pretty-printing
	CMD_SOURCE, // source
	CMD_FILE_EXEC_AND_SYMBOLS, // -file-exec-and-symbols
	CMD_HANDLE, // handle
};


void getCommandsHelp(wstring_vector & res, bool forMi);

struct MICommand {
	uint64_t requestId;
	MiCommandId commandId;
	std::wstring threadGroupId;
	unsigned threadId;
	unsigned frameId;
	PrintLevel printLevel;// 0=--no-values 1=--all-values 2=--simple-values
	bool skipUnavailable;//--skip-unavailable
	bool noFrameFilters; //--no-frame-filters
	/// true if command is prefixed with single -
	bool miCommand;
	/// original command text
	std::wstring commandText;
	/// command name string
	std::wstring commandName;
	/// tail after command till end of line
	std::wstring tail;
	/// individual parameters from tail
	wstring_vector params;
	/// named parameters - pairs (key, value)
	param_vector namedParams;
	/// parameters with values w/o names
	wstring_vector unnamedValues;

	// debug dump
	std::wstring dumpCommand();

	/// returns true if there is specified named parameter in cmd
	bool hasParam(std::wstring name);
	// find parameter by name
	std::wstring findParam(std::wstring name);
	std::wstring unnamedValue(unsigned index = 0) { return index < unnamedValues.size() ? unnamedValues[index] : std::wstring(); }
	// get parameter --thread-id
	uint64_t getUlongParam(std::wstring name, uint64_t defValue = 0);

	MICommand();
	~MICommand();
	// parse MI command, returns true if successful
	bool parse(std::wstring line);
private:
	void handleKnownParams();
};

