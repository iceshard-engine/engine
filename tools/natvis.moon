import Command, option from require "ice.command"
import Exec from require "ice.tools.exec"
import Json from require "ice.util.json"

import loadstring from require "moonscript"

py3_script = [[import mmh3; %s]]
py3_hash = [[print('[\'%s\']: {}'.format(mmh3.hash('%s', signed=False, seed=0x428639DA)));]]

class NatvisCommand extends Command
    @arguments {
    }

    prepare: (args, project) =>
        os.chdir "source/code"

    iterate_over_headers: (path, shard_names) =>
        return unless path\match "%.hxx"

        if f = io.open path, "rb"
            for line in f\lines!
                var, val = line\match 'static constexpr ice::Shard ([%w_:]+) = "([%w/-]+)"_shard'
                if var and val
                    print "Warning: Shard with this name '#{var}' already exists!" if shard_names.shard[var]
                    shard_names.shard[var] = val
                    continue

                var, val = line\match 'Constant_ShardPayloadID<([%w_:%*]+)> = ice::shard_payloadid%("([%w_:*]+)"%)'
                if var or val
                    shard_names.payloadid[var] = val

            f\close!

    iterate_over_directory: (path, shard_names) =>
        for name, mode in os.listdir path, 'mode'
            continue if name == '.' or name == '..'

            @iterate_over_directory "#{path}/#{name}", shard_names if mode == 'directory'
            @iterate_over_headers "#{path}/#{name}", shard_names

    execute: (args) =>
        names = { shard: { }, payloadid: {} }
        @iterate_over_directory ".", names

        generate_hashes = (names) ->
            all_hashes = ""
            for name, value in pairs names
                all_hashes ..= string.format(py3_hash, value, value)

            cmd = 'python3 -c "' .. string.format(py3_script, all_hashes) .. '"'
            p = io.popen cmd, 'r'
            return { } unless p

            hash_results = p\read '*all'
            p\close!

            (loadstring "{\n" .. hash_results .. "\n}")!

        sort_results = (hash_results) ->
            sorted_list = { }
            for name, hash in pairs hash_results
                table.insert sorted_list, { :name, :hash }

            table.sort sorted_list, (left, right) -> left.name < right.name
            return sorted_list

        hashes = sort_results generate_hashes names.shard
        payload_hashes = sort_results generate_hashes names.payloadid

        if f = io.open "./core/core/natvis/shard_names.natvis", "wb+"
            f\write '<?xml version="1.0" encoding="utf-8"?>\n'
            f\write '<AutoVisualizer xmlns="http://schemas.microsoft.com/vstudio/debugger/natvis/2010">\n'
            f\write '    <Type Name="ice::Shard">\n'
            f\write '        <DisplayString Condition="name == ' .. hash .. '">Shard {{ ' .. name .. ' }}</DisplayString>\n' for { :name, :hash } in *hashes
            f\write '        <DisplayString>Shard {{ {name,h}, unknown_name }}</DisplayString>\n'
            f\write '        <Expand>\n'
            f\write '            <Synthetic Name="[payload type]">\n'
            f\write '                <DisplayString Condition="payload_id == ' .. hash .. '">' .. name .. '</DisplayString>\n' for { :name, :hash } in *payload_hashes
            f\write '                <DisplayString Condition="payload_id == 0">{payload_id}, type_not_set</DisplayString>\n'
            f\write '                <DisplayString>{payload_id}, unknown_type</DisplayString>\n'
            f\write '            </Synthetic>\n'
            f\write '            <Item Name="[payload]" Optional="true" Condition="payload_id == ' .. hash .. '">*(' .. name .. '*)&amp;payload</Item>\n' for { :name, :hash } in *payload_hashes
            f\write '        </Expand>\n'
            f\write '    </Type>\n'
            f\write '</AutoVisualizer>\n'
            f\close!

{ :NatvisCommand }
