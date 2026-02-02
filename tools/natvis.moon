import Command, option from require "ice.command"
import Exec from require "ice.tools.exec"
import Json from require "ice.util.json"
import Dir from require "ice.core.fs"

import loadstring from require "moonscript"

py3_script = [[import mmh3; %s]]
py3_hash_shard = [[print('[\'%s\']: {}'.format(mmh3.hash('%s', signed=False, seed=0x77a23ab1)));]]
py3_hash_payload = [[print('[\'%s\']: {}'.format(mmh3.hash('%s', signed=False, seed=0x3ab177a2)));]]

class NatvisCommand extends Command
    @arguments {
    }

    prepare: (args, project) =>
        os.chdir "source/code"

    iterate_over_headers: (path, shard_names, locations) =>
        return unless path\match "%.hxx"

        if f = io.open path, "rb"
            idx = 0
            for line in f\lines!
                idx += 1
                var, val = line\match 'static constexpr ice::Shard ([%w_:]+) = "([%w/-]+)[%w_:*` ]*"_shard'
                var, val = line\match 'static constexpr ice::ShardID ([%w_:]+) = "([%w/-]+)[%w_:*` ]*"_shardid' unless var and val
                if var and val
                    print "Warning: Shard with this name '#{var}' already exists!" if shard_names.shard[var]
                    shard_names.shard[var] = val
                    locations[val] = "#{path}(#{idx})"
                    continue

                var, val = line\match 'Constant_ShardPayloadID<([%w_:* ]+)> = ice::shard_payloadid%("([%w_:* ]+)"%)'
                if var or val
                    shard_names.payloadid[var] = val
                    locations[val] = "#{path}(#{idx})"

            f\close!

    execute: (args) =>
        locations = { }
        names = { shard: { }, payloadid: {} }

        for path in Dir\list ".", recursive:true
            @iterate_over_headers path, names, locations

        generate_hashes = (names, py3_hash_script) ->
            all_hashes = ""
            for name, value in pairs names
                all_hashes ..= string.format(py3_hash_script, value, value)

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

        hashes = sort_results generate_hashes names.shard, py3_hash_shard
        payload_hashes = sort_results generate_hashes names.payloadid, py3_hash_payload

        if f = io.open "./core/core/natvis/shard_names.natvis", "wb+"
            f\write '<?xml version="1.0" encoding="utf-8"?>\n'
            f\write '<AutoVisualizer xmlns="http://schemas.microsoft.com/vstudio/debugger/natvis/2010">\n'
            f\write '    <Type Name="ice::Shard">\n'
            f\write '        <DisplayString Condition="id.name.value == ' .. hash .. '">Shard {{ ' .. name .. ' }}</DisplayString>\n' for { :name, :hash } in *hashes
            f\write '        <DisplayString>Shard {{ {id.name.value,h}, unknown_name }}</DisplayString>\n'
            f\write '        <Expand>\n'
            f\write '            <Synthetic Name="[payload type]">\n'
            f\write '                <DisplayString Condition="id.payload.value == ' .. hash .. '">' .. name .. '</DisplayString>\n' for { :name, :hash } in *payload_hashes
            f\write '                <DisplayString Condition="id.payload.value == 0">{id.payload.value}, type_not_set</DisplayString>\n'
            f\write '                <DisplayString>{id.payload.value}, unknown_type</DisplayString>\n'
            f\write '            </Synthetic>\n'
            for { :name, :hash } in *payload_hashes
                f\write '            <!-- defined-at:"'.. locations[name] .. '" -->\n'
                f\write '            <Item Name="[payload]" Optional="true" Condition="id.payload.value == ' .. hash .. '">*(' .. name .. '*)&amp;payload.value</Item>\n'
            f\write '        </Expand>\n'
            f\write '    </Type>\n'
            f\write '</AutoVisualizer>\n'
            f\close!

{ :NatvisCommand }
