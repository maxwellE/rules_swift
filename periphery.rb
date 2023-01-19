require "open3"

bazel_path = ARGV.first
tmpdir = `mktemp -d`.strip

`#{bazel_path} build //examples/xplatform/swift_import/... --build_event_text_file="#{tmpdir}/periphery_bep.text" --features=swift.index_while_building --output_groups=+swift_index_store`

indexstores = File.read("#{tmpdir}/periphery_bep.text").scan(/uri: "file:\/\/(.+\.indexstore)/).compact{ $0[0] }.uniq.flatten
remapped_indexstores = []

for indexstore in indexstores
    remapped_indexstore = tmpdir + "/" + File.basename(indexstore) + ".remapped"
    `#{bazel_path} run @build_bazel_rules_swift_index_import//:index_import -- --remap="^/.+/build_bazel_rules_swift"="$PWD" "#{indexstore}" "#{remapped_indexstore}"`
    remapped_indexstores << remapped_indexstore
end

indexstores_params = remapped_indexstores.map { |indexstore| "--index-store-path \"#{indexstore}\"" }.join(" ")

puts Open3.capture2("#{bazel_path} run @com_github_peripheryapp//:periphery -- scan --skip-build --file-targets-path \"#{Dir.pwd}/file-targets-path.txt\" #{indexstores_params}")
