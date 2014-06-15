Vagrant::Config.run do |config|
  config.vm.box = 'lucid32'
  config.vm.box_url = 'http://files.vagrantup.com/lucid32.box'

  config.vm.provision "ansible" do |ansible|
    ansible.playbook = "provisioning/site.yml"
  end
end
