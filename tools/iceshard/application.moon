require "iceshard.util.os"


argparse = require "argparse"

option = (tab) -> func:'option', args:{ tab.name, tab.description, tab.default, tab.convert, tab.args, nil }
flag = (tab) -> func:'flag', args:{ tab.name, tab.description, tab.default, tab.convert, nil, tab.count }

class Command
    new: (@parser) =>
        -- Add all defined arguments
        for { :func, :args } in *@@arguments
            @parser[func] @parser, unpack args

    execute: =>


class Application
    new: =>
        @script_file = arg[1]
        @parser = argparse @@name, @@description, @@epilog
        @parser\require_command false
        @parser\command_target "command"

        -- Add all defined arguments
        for { :func, :args } in *@@arguments or { }
            @parser[func] @parser, unpack args

        -- Go through all defined actions (table values)
        @commands = { }
        for name, command_clazz in pairs @@commands or { }
            command = command_clazz @parser\command name, command_clazz.description, command_clazz.epilog

            -- Save the object
            @commands[name] = command

    run: =>
        args = @parser\parse arg

        if args.command
            @commands[args.command]\execute args

        else
            @execute args

    execute: =>

{ :Command, :Application, :option, :flag }
