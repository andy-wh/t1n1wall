<?php
/*
	$Id: services.inc 547 2013-11-14 12:40:38Z mkasper $
	part of m0n0wall (http://m0n0.ch/wall)
	
	Copyright (C) 2003-2007 Manuel Kasper <mk@neon1.net>.
	All rights reserved.
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	
	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.
	
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	
	THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
	AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
	OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

/* include all configuration functions */
require_once("functions.inc");

function services_dhcp6c_stop() {
	/* kill any running dhcp6c */
	killbypid("{$g['varrun_path']}/dhcp6c.pid");
	mwexec('/sbin/sysctl net.inet6.ip6.accept_rtadv=0');
}

function services_dhcp6c_configure() {
	global $config, $g;
	
	/* kill any running dhcp6c */
	killbypid("{$g['varrun_path']}/dhcp6c.pid");
	
	/* write dhcp6c_duid */
	if ($config['interfaces']['wan']['v6duid']) {
		
		$dhcp6duid = $config['interfaces']['wan']['v6duid'] ;
		$fd = fopen("{$g['vardb_path']}/dhcp6c_duid", "w");
		if (!$fd) {
			printf("Error: cannot open dhcp6c_duid in services_dhcp6c_configure().\n");
			return 1;
		}
	
		$duidbytes = explode(":",$dhcp6duid);
	
		foreach ($duidbytes as $duidpiece) {
		    $tmpdhcp6duid .= pack("H*", $duidpiece);    
		}

		fwrite($fd, chr(strlen($tmpdhcp6duid)) . chr(0) . $tmpdhcp6duid);
		fclose($fd);
	}
	
	/* write dhcp6c.conf */
	$wanif = get_real_wan_interface();
	$fd = fopen("{$g['varetc_path']}/dhcp6c.conf", "w");
	if (!$fd) {
		printf("Error: cannot open dhcp6c.conf in services_dhcp6c_configure().\n");
		return 1;
	}
	
	if ($config['interfaces']['wan']['ipaddr6'] == "DHCP") {
		mwexec('/sbin/sysctl net.inet6.ip6.accept_rtadv=1');
		$d6wans = <<<EOD
		request domain-name-servers;
		script "/etc/dhcp6c-exit-hooks";
		send ia-na 0;

EOD;
	}
	$p = 1;
	if ($config['interfaces']['lan']['ipaddr6'] == "DHCP-PD") {
		$d6wanm = "\t\tsend ia-pd 0;\n";
		$d6pds = "id-assoc pd 0 {\n";
		$d6pde = "};\n";
		if (isset($config['interfaces']['lan']['slaid'])) {
			$d6pdi = $config['interfaces']['lan']['slaid'];
		} else {
			$d6pdi = $p;
		}
		if (isset($config['interfaces']['lan']['slalen'])) {
			$d6pdl = $config['interfaces']['lan']['slalen'];
		} else {
			$d6pdl = 16;
		}
		$d6lan .= <<<EOD
		prefix-interface {$config['interfaces']['lan']['if']} {
			sla-id {$d6pdi};
			sla-len {$d6pdl};
		};

EOD;
	$p++;
	}
			
	for ($i = 1; isset($config['interfaces']['opt' . $i]); $i++) {
		if ($config['interfaces']['opt' . $i]['ipaddr6'] == "DHCP-PD") {
			$d6wanm = "\t\tsend ia-pd 0;\n";
			$d6pds = "id-assoc pd 0 {\n";
			$d6pde = "};\n";
			if (isset($config['interfaces']['opt' . $i]['slaid'])) {
				$d6pdi = $config['interfaces']['opt' . $i]['slaid'];
			} else {
				$d6pdi = $p;
			}
			if (isset($config['interfaces']['opt' . $i]['slalen'])) {
				$d6pdl = $config['interfaces']['opt' . $i]['slalen'];
			} else {
				$d6pdl = 16;
			}
			$d6opt .= <<<EOD
		prefix-interface {$config['interfaces']['opt' . $i]['if']} {
			sla-id {$d6pdi};
			sla-len {$d6pdl};
		};

EOD;
			$p++;
		}
	}

	$dhcp6cconf = "interface {$wanif} {\n" . $d6wans . $d6wanm . "};\n" . $d6pds . $d6lan . $d6opt . $d6pde ;
 	$dhcp6cconf .= <<<EOD
id-assoc na 0 {
};
EOD;
	
	fwrite($fd, $dhcp6cconf);
	fclose($fd);

	/* fire up dhcp6c */
	$wanif = get_real_wan_interface();
	mwexec("/usr/local/sbin/dhcp6c -c {$g['varetc_path']}/dhcp6c.conf -p {$g['varrun_path']}/dhcp6c.pid {$wanif}" );

	return 0;

}

function services_dhcp6s_configure() {
	global $config, $g;
	
	/* kill any running dhcp6s */
	$dircont = opendir($g['varrun_path']);
	while ($file = readdir($dircont)) {
		if (preg_match("/dhcp6s/",$file)) killbypid("{$g['varrun_path']}/$file");
	}
	closedir($dircont);

	killbypid("{$g['varrun_path']}/dhcp6s.pid");
	if (!file_exists("{$g['varrun_path']}/dhcp6sctlkey"))  mwexec("touch {$g['varrun_path']}/dhcp6sctlkey");
	
	$syscfg = $config['system'];
	$dhcp6scfg = $config['dhcpd'];
	
	/* DHCPv6 enabled on any interfaces? */
	$dhcp6senable = false;
	foreach ($dhcp6scfg as $dhcpif => $dhcpifconf) {
		if (isset($dhcpifconf['v6enable']) && (($dhcpif == "lan") || (isset($config['interfaces'][$dhcpif]['ipaddr6']) && $config['interfaces'][$dhcpif]['if'] && (!$config['interfaces'][$dhcpif]['bridge']))))
			$dhcp6senable = true;
	}
	
	if (!$dhcp6senable)
		return 0;
	
	if ($g['booting'])
		echo "Starting DHCPv6 service... ";
	else
		sleep(1);

	$dhcp6sifs = array();
	foreach ($dhcp6scfg as $dhcpif => $dhcpifconf) {
		$ifcfg = $config['interfaces'][$dhcpif];
		
		/* write dhcp6s.conf */
		$fd = fopen("{$g['varetc_path']}/dhcp6s.{$ifcfg['if']}.conf", "w");
		if (!$fd) {
			printf("Error: cannot open dhcp6s.conf in services_dhcp6s_configure().\n");
			return 1;
		}
		$dhcp6sconf = <<<EOD
	
option domain-name "{$syscfg['domain']}";

EOD;
		if (!isset($dhcpifconf['v6enable']))
			continue;
		$v6subnet = gen_subnet6($ifcfg['ipaddr6'], $ifcfg['subnet6']);
		$dnscfg = "";
		unset($ifconfiginfo);
		exec("/sbin/ifconfig " . $ifcfg['if'], $ifconfiginfo);
		$ifmacaddr = "01:01:01:01:01:01";
		foreach ($ifconfiginfo as $ici) {	
			if (preg_match("/ether (\S+)/", $ici, $matches)) {
				$ifmacaddr = $matches[1];
			} 
		}
			
		if (is_array($dhcpifconf['dnsserver']) && ($dhcpifconf['dnsserver'][0])) {
			list($dnsconfig['dns1'],$dnsconfig['dns2'],$dnsconfig['dns3']) = $dhcpifconf['dnsserver'];
			if (is_ipaddr6($dnsconfig['dns1']))	$dnscfg .= "option domain-name-servers " . $dnsconfig['dns1'] . ";\n";
			if (is_ipaddr6($dnsconfig['dns2']))	$dnscfg .= "option domain-name-servers " . $dnsconfig['dns2'] . ";\n";
			if (is_ipaddr6($dnsconfig['dns3']))	$dnscfg .= "option domain-name-servers " . $dnsconfig['dns3'] . ";\n";
		} else if (isset($config['dnsmasq']['enable'])) {
			$dnscfg .= "option domain-name-servers " . $ifcfg['ipaddr6'] . ";\n";
		} else if (is_array($syscfg['dnsserver']) && ($syscfg['dnsserver'][0])) {
			list($dnsconfig['dns1'],$dnsconfig['dns2'],$dnsconfig['dns3']) = $syscfg['dnsserver'];
			if (is_ipaddr6($dnsconfig['dns1']))	$dnscfg .= "option domain-name-servers " . $dnsconfig['dns1'] . ";\n";
			if (is_ipaddr6($dnsconfig['dns2']))	$dnscfg .= "option domain-name-servers " . $dnsconfig['dns2'] . ";\n";
			if (is_ipaddr6($dnsconfig['dns3']))	$dnscfg .= "option domain-name-servers " . $dnsconfig['dns3'] . ";\n";
		}
		$dhcp6sconf .= <<<EOD
$dnscfg

interface {$ifcfg['if']} {
	preference 255;
	address-pool pool{$ifcfg['if']} {$dhcpifconf['v6defaultleasetime']} {$dhcpifconf['v6maxleasetime']};
};

pool pool{$ifcfg['if']} {
        range {$dhcpifconf['v6range']['from']} to {$dhcpifconf['v6range']['to']} ;
};

host m0n0wall {
        duid {$ifmacaddr};
        address {$ifcfg['ipaddr6']} infinity;
};

EOD;
		/* add static mappings */
		if (is_array($dhcpifconf['staticmap'])) {
			$i = 0;
			foreach ($dhcpifconf['staticmap'] as $sm) {
				if (is_ipaddr6($sm['ipaddr'])){
					$dhcp6sconf .= <<<EOD
host reserved{$i} {
        duid {$sm['mac']};
        address {$sm['ipaddr']} {$dhcpifconf['v6defaultleasetime']};
};

EOD;
					$i++;
				}
			}
		}
			
		$dhcp6sifs[] = $ifcfg['if'];
		fwrite($fd, $dhcp6sconf);
		fclose($fd);
	}
	
	/* fire up dhcp6s for each if */
	$i = 64705;
	foreach ($dhcp6sifs as $dhcp6sif) {
		mwexec("/usr/local/sbin/dhcp6s -c {$g['varetc_path']}/dhcp6s.{$dhcp6sif}.conf -k {$g['varrun_path']}/dhcp6sctlkey -p {$i} -P {$g['varrun_path']}/dhcp6s.{$dhcp6sif}.pid {$dhcp6sif}" );
		$i++;
	}
		
	if (!$g['booting']) {
		filter_configure();
	} else
		echo "done\n";
	
	return 0;
}
function services_dhcpd_configure() {
	global $config, $g;

	
	$syscfg = $config['system'];
	$dhcpdcfg = $config['dhcpd'];
	
	/* DHCP enabled on any interfaces? */
	$dhcpdenable = false;
	foreach ($dhcpdcfg as $dhcpif => $dhcpifconf) {
		if (isset($dhcpifconf['enable']) &&
			(($dhcpif == "lan") ||
			(isset($config['interfaces'][$dhcpif]['enable']) &&
			$config['interfaces'][$dhcpif]['if'] && (!$config['interfaces'][$dhcpif]['bridge']))))
			$dhcpdenable = true;
	}
	
	if (!$dhcpdenable)
		return 0;

	if ($g['booting'])
		echo "Starting DHCP service... ";
	else
		sleep(1);
	
	/* write dhcpd.conf */
	if (file_exists("{$g['varetc_path']}/dnsmasq/dhcpd.conf")) {
		unlink("{$g['varetc_path']}/dnsmasq/dhcpd.conf");
	}	

	$fd = fopen("{$g['varetc_path']}/dnsmasq/dhcpd.conf", "w");
	if (!$fd) {
		printf("Error: cannot open dhcpd.conf in services_dhcpd_configure().\n");
		return 1;
	}
	
	/* kill any running dnsmasq */
	sigkillbypid("{$g['varrun_path']}/dnsmasq.pid", "TERM");

	$dhcpdconf = <<<EOD
dhcp-option=option:domain-name, "{$syscfg['domain']}"
dhcp-authoritative
dhcp-leasefile=/var/db/dnsmasq.dhcpd.leases

EOD;
	
	$dhcpdifs = array();
	foreach ($dhcpdcfg as $dhcpif => $dhcpifconf) {
		
		$ifcfg = $config['interfaces'][$dhcpif];
		
		if (!isset($dhcpifconf['enable']) ||
			(($dhcpif != "lan") &&
			(!isset($ifcfg['enable']) || !$ifcfg['if'] || $ifcfg['bridge'])))
			continue;
			
		$subnet = gen_subnet($ifcfg['ipaddr'], $ifcfg['subnet']);
		$subnetmask = gen_subnet_mask($ifcfg['subnet']);
		
		$dhcpdconf .= "dhcp-range=set:{$dhcpif},{$dhcpifconf['range']['from']},{$dhcpifconf['range']['to']}";

		if (isset($dhcpifconf['denyunknown'])) 
		   $dhcpdconf .= ",static";

		if ($dhcpifconf['defaultleasetime'])
			$dhcpdconf .= ",{$dhcpifconf['defaultleasetime']}s";

		$dhcpdconf .= "\n";

		if ($dhcpifconf['domain']) {
			$dhcpdconf .= " dhcp-option=tag:{$dhcpif},option:domain-name,{$dhcpifconf['domain']}\n";
		}
		
		$dnsservers = array();
		if (is_array($dhcpifconf['dnsserver']) && ($dhcpifconf['dnsserver'][0]))
			$dnsservers = $dhcpifconf['dnsserver'];
		else if (isset($config['dnsmasq']['enable']))
			$dnsservers = array($ifcfg['ipaddr']);
		else if (is_array($syscfg['dnsserver']) && ($syscfg['dnsserver'][0]))
			$dnsservers = $syscfg['dnsserver'];
		
		$dnsservers = array_filter($dnsservers, "is_ipaddr");
		if (!empty($dnsservers))
			$dhcpdconf .= " dhcp-option=tag:{$dhcpif},option:dns-server," . join(",", $dnsservers) . "\n";
		
		if ($dhcpifconf['gateway'])
			$routers = $dhcpifconf['gateway'];
		else
			$routers = $ifcfg['ipaddr'];

		$dhcpdconf .= " dhcp-option=tag:{$dhcpif},option:router,{$routers}\n";
	
		if (is_array($dhcpifconf['winsserver']) && $dhcpifconf['winsserver'][0]) {
			$dhcpdconf .= " dhcp-option=tag:{$dhcpif},option:netbios-ns," . join(",", $dhcpifconf['winsserver']) . "\n";
			$dhcpdconf .= "	dhcp-option=tag:{$dhcpif},option:netbios-nodetype,8\n";
		}
		
		if ($dhcpifconf['next-server'])
			$dhcpdconf .= "	dhcp-option=tag:{$dhcpif},option:tftp-server,{$dhcpifconf['next-server']};\n";
		if ($dhcpifconf['filename'])
			$dhcpdconf .= "	dhcp-option=tag:{$dhcpif},option:boot-filename,\"{$dhcpifconf['filename']}\"\n";
			
		/* add static mappings */
		if (is_array($dhcpifconf['staticmap'])) {
		
			$i = 0;
			foreach ($dhcpifconf['staticmap'] as $sm) {
				if (!is_ipaddr6($sm['ipaddr'])) {
					$dhcpdconf .= "	dhcp-host={$sm['mac']},set:s_{$dhcpif}_{$i},{$sm['ipaddr']}";
					if ($dhcpifconf['defaultleasetime'])
						$dhcpdconf .= ",{$dhcpifconf['defaultleasetime']}s";
					$dhcpdconf .= "\n";

					if ($sm['next-server'])
						$dhcpdconf .= "	 dhcp-option=tag:s_{$dhcpif}_{$i},option:tftp-server,{$sm['next-server']}\n";
					if ($sm['filename'])
						$dhcpdconf .= "	 dhcp-option=tag:s_{$dhcpif}_{$i},option:boot-filename, \"{$sm['filename']}\"\n";
					$i++;
				}
			}
		}
		
		$dhcpdifs[] = $ifcfg['if'];
	}

	fwrite($fd, $dhcpdconf);
	fclose($fd);

	/* run dnsmasq */
	mwexec("/usr/local/sbin/dnsmasq --conf-dir={$g['varetc_path']}/dnsmasq");
		
	if (!$g['booting']) {
		filter_configure();
	} else
		echo "done\n";
	
	return 0;
}

function services_dhcrelay_configure() {
	global $config, $g;
	
	
	$dhcrelaycfg = $config['dhcrelay'];
	
	/* DHCPRelay enabled on any interfaces? */
	$dhcrelayenable = false;
	
	if (is_array($dhcrelaycfg)) {
		foreach ($dhcrelaycfg as $dhcrelayif => $dhcrelayifconf) {
			if (isset($dhcrelayifconf['enable']) &&
				(($dhcrelayif == "lan") ||
				(isset($config['interfaces'][$dhcrelayif]['enable']) &&
				$config['interfaces'][$dhcrelayif]['if'] && (!$config['interfaces'][$dhcrelayif]['bridge'])))) {
				
				$dhcrelayenable = true;
				break;
			}
		}
	}
	
	if (!$dhcrelayenable)
		return 0;
	
	/* write dhcprelay.conf */
	if (file_exists("{$g['varetc_path']}/dnsmasq/dhcprelay.conf")) {
		unlink("{$g['varetc_path']}/dnsmasq/dhcprelay.conf");
	}	

	$fd = fopen("{$g['varetc_path']}/dnsmasq/dhcprelay.conf", "w");
	if (!$fd) {
		printf("Error: cannot open dhcprelay.conf in services_dhcrelay_configure().\n");
		return 1;
	}

	/* kill any running dnsmasq */
	sigkillbypid("{$g['varrun_path']}/dnsmasq.pid", "TERM");

	if ($g['booting'])
		echo "Starting DHCP relay service... ";
	else
		sleep(1);
	
	$dhcprconf = "";
	$dhcrelayifs = array();
	foreach ($dhcrelaycfg as $dhcrelayif => $dhcrelayifconf) {
		
		$ifcfg = $config['interfaces'][$dhcrelayif];
		
		if (!isset($dhcrelayifconf['enable']) ||
			(($dhcrelayif != "lan") &&
			(!isset($ifcfg['enable']) || !$ifcfg['if'] || $ifcfg['bridge'])))
			continue;

		$dhcprconf .= "dhcp-relay={$ifcfg['ipaddr']},{$dhcrelaycfg['server']}\n";
	}
	
	if (!isset($destif)) 
		$destif = $config['interfaces']['wan']['if'];
	
	$dhcrelayifs[] = $destif;


	fwrite($fd, $dhcprconf);
	fclose($fd);

	/* run dnsmasq */
	mwexec("/usr/local/sbin/dnsmasq --conf-dir={$g['varetc_path']}/dnsmasq");
		
	if (!$g['booting']) {
		filter_configure();
	} else
		echo "done\n";
	
	return 0;
}

function services_dyndns_reset() {
	global $config, $g;

	if (file_exists("{$g['vardb_path']}/ez-ipupdate.cache")) {
		unlink("{$g['vardb_path']}/ez-ipupdate.cache");
	}	
	
	if (file_exists("{$g['conf_path']}/ez-ipupdate.cache")) {
		conf_mount_rw();
		unlink("{$g['conf_path']}/ez-ipupdate.cache");
		conf_mount_ro();
	}
	
	return 0;
}

function services_dyndns_configure() {
	global $config, $g;
	
	/* kill any running ez-ipupdate */
	/* ez-ipupdate needs SIGQUIT instead of SIGTERM */
	sigkillbypid("{$g['varrun_path']}/ez-ipupdate.pid", "QUIT");
	
	$dyndnscfg = $config['dyndns'];
	
	if (isset($dyndnscfg['enable'])) {
	
		if ($g['booting'])
			echo "Starting DynDNS client... ";
		else
			sleep(1);
		
		/* determine WAN interface name */
		$wanif = get_real_wan_interface();
		
		/* write ez-ipupdate.conf */
		$fd = fopen("{$g['varetc_path']}/ez-ipupdate.conf", "w");
		if (!$fd) {
			printf("Error: cannot open ez-ipupdate.conf in services_dyndns_configure().\n");
			return 1;
		}
			
		$ezipupdateconf = <<<EOD
service-type={$dyndnscfg['type']}
user={$dyndnscfg['username']}:{$dyndnscfg['password']}
host={$dyndnscfg['host']}
interface=$wanif
max-interval=2073600
pid-file={$g['varrun_path']}/ez-ipupdate.pid
cache-file={$g['vardb_path']}/ez-ipupdate.cache
execute=/etc/rc.dyndns.storecache
daemon

EOD;

		/* enable server[:port]? */
		if ($dyndnscfg['server']) {
			if ($dyndnscfg['port'])
				$ezipupdateconf .= "server={$dyndnscfg['server']}:{$dyndnscfg['port']}\n";
			else
				$ezipupdateconf .= "server={$dyndnscfg['server']}\n";
		}

		/* enable MX? */
		if ($dyndnscfg['mx']) {
			$ezipupdateconf .= "mx={$dyndnscfg['mx']}\n";
		}
		
		/* enable wildcards? */
		if (isset($dyndnscfg['wildcard'])) {
			$ezipupdateconf .= "wildcard\n";
		}

		fwrite($fd, $ezipupdateconf);
		fclose($fd);
		
		/* if we're booting, copy the cache file from /conf */
		if ($g['booting']) {
			if (file_exists("{$g['conf_path']}/ez-ipupdate.cache")) {
				copy("{$g['conf_path']}/ez-ipupdate.cache", "{$g['vardb_path']}/ez-ipupdate.cache");
			}
		}
		
		/* run ez-ipupdate */
		mwexec("/usr/local/bin/ez-ipupdate -c {$g['varetc_path']}/ez-ipupdate.conf");
		
		if ($g['booting'])
			echo "done\n";
	}
	
	return 0;
}

function services_dnsmasq_configure() {
	global $config, $g;

	/* kill any running dnsmasq */
	sigkillbypid("{$g['varrun_path']}/dnsmasq.pid", "TERM");

	if (isset($config['dnsmasq']['enable'])) {
	
		if ($g['booting'])
			echo "Starting DNS forwarder... ";
		else
			sleep(1);

		/* generate hosts file */
		system_hosts_generate();
		
		/* generate dnsmasq.conf */
		$fd = fopen("{$g['varetc_path']}/dnsmasq/dnsmasq.conf", "w");
		if (!$fd) {
			printf("Error: cannot open dnsmasq.conf in services_dnsmasq_configure().\n");
			return 1;
		}
		$dnsmasqconf = "edns-packet-max=4096\n";
		$dnsmasqconf .= "log-facility=local7\n";
		$dnsmasqconf .= "except-interface=" . get_real_wan_interface() . "\n";
		if (ipv6enabled())
			$dnsmasqconf .= "except-interface=" . get_real_wan_interface(true) . "\n";
		
		if (isset($config['dnsmasq']['regdhcp'])) {
			$dnsmasqconf .= "domain={$config['system']['domain']}\n";
			$dnsmasqconf .= "dhcp-fqdn\n";
		}

		$dnsmasqcfg = $config['dnsmasq'];
		if (is_array($dnsmasqcfg['hosts'])) {
			foreach ($dnsmasqcfg['hosts'] as $host) {
				if (($host['host']) && ($host['host'] == '*')) {
					$dnsmasqconf .= "address=/{$host['domain']}/{$host['ip']}\n";
				}
				if ($host['aliases']) {
					$aliases = explode(' ', $host['aliases']);
					foreach ($aliases as $alias) {
						$dnsmasqconf .= "cname={$alias},{$host['host']}\n";
						$dnsmasqconf .= "cname={$alias}.{$host['domain']}," .
								"{$host['host']}.{$host['domain']}\n";
					}
				}
				if (isset($host['mx'])) {
					$dnsmasqconf .=	"mx-host={$host['domain']}," .
									"{$host['host']}.{$host['domain']}\n";
				}
			}
		}

		if (isset($config['dnsmasq']['allservers'])) {
			$dnsmasqconf .= "all-servers\n";
		}
		
		if (isset($config['dnsmasq']['strictorder'])) {
			$dnsmasqconf .= "strict-order\n";
		}
		
		if (isset($config['dnsmasq']['domainoverrides']) && is_array($config['dnsmasq']['domainoverrides'])) {
			foreach($config['dnsmasq']['domainoverrides'] as $override) {
				$dnsmasqconf .= "server=/{$override['domain']}/{$override['ip']}\n";
			}
		}

		if (isset($config['dnsmasq']['stoprebind'])) {
			$dnsmasqconf .= "stop-dns-rebind\n";
		}
		if (isset($config['dnsmasq']['log'])) {
			$dnsmasqconf .= "log-queries\n";
		}

		fwrite($fd, $dnsmasqconf);
		fclose($fd);

		/* run dnsmasq */
		mwexec("/usr/local/sbin/dnsmasq --conf-dir={$g['varetc_path']}/dnsmasq");

		if ($g['booting'])
			echo "done\n";
	}
	
	if (!$g['booting']) {
		services_dhcpd_configure();
		filter_configure();
	}

	return 0;
}

function services_snmpd_configure() {
	global $config, $g;

	/* kill any running snmpd */
	sigkillbypid("{$g['varrun_path']}/snmpd.pid", "TERM");

	if (isset($config['snmpd']['enable'])) {
	
		if ($g['booting'])
			echo "Starting SNMP agent... ";

		/* generate snmpd.conf */
		$fd = fopen("{$g['varetc_path']}/snmpd.conf", "w");
		if (!$fd) {
			printf("Error: cannot open snmpd.conf in services_snmpd_configure().\n");
			return 1;
		}
		
		$version = chop(file_get_contents("{$g['etc_path']}/version"));
		$specplatform = system_identify_specific_platform();
		$sysDescr = "m0n0wall " . php_uname("n") . " " . $version . " " .
			$specplatform['name'] . " " . php_uname("s") . " " . php_uname("r") .
			" " . php_uname("m");
		
		$snmpdconf = <<<EOD
sysdescr $sysDescr
syslocation {$config['snmpd']['syslocation']}
syscontact {$config['snmpd']['syscontact']}
rocommunity {$config['snmpd']['rocommunity']}

EOD;

		/* setup overides to make ifAlias work, find ifIndex from netstat */

		exec("/usr/bin/netstat -inW -f link", $linkinfo);
		array_shift($linkinfo);

		foreach ($linkinfo as $link) {
			$alink = preg_split("/\s+|\#|\>/", $link);
			$ifname = chop($alink[0]);
			$ifLink = chop($alink[3]);
			$friendlyIfName = convert_real_interface_to_friendly_interface_name($ifname);
			$snmpdconf .= "override ifAlias." . $ifLink . " octet_str \"" . $friendlyIfName . "\"\n";
		}

		if (isset($config['snmpd']['bindlan'])) {
			$snmpdconf .= "agentaddress udp:161@{$config['interfaces']['lan']['ipaddr']}\n";
		}

		fwrite($fd, $snmpdconf);
		fclose($fd);

		/* run snmpd */
		mwexec("/usr/local/sbin/snmpd -c {$g['varetc_path']}/snmpd.conf" .
			" -P {$g['varrun_path']}/snmpd.pid");

		if ($g['booting'])
			echo "done\n";
	}

	return 0;
}

function services_proxyarp_configure() {
	global $config, $g;

	/* kill any running choparp */
	killbyname("choparp");
	
	if (is_array($config['proxyarp']) && count($config['proxyarp'])) {
		
		$paa = array();
		
		/* group by interface */
		foreach ($config['proxyarp']['proxyarpnet'] as $paent) {
			if ($paent['interface'])
				$if = $paent['interface'];
			else
				$if = "wan";
			
			if (!is_array($paa[$if]))
				$paa[$if] = array();
			
			$paa[$if][] = $paent;
		} 
	
		foreach ($paa as $paif => $paents) {
			if ($paif == "wan" && !(is_ipaddr($config['interfaces']['wan']['ipaddr']) ||
					($config['interfaces']['wan']['ipaddr'] == "dhcp")))
				continue;
			
			$args = $config['interfaces'][$paif]['if'] . " auto";
		
			foreach ($paents as $paent) {
				if (isset($paent['network']))
					$args .= " " . escapeshellarg($paent['network']);
				else if (isset($paent['range']))
					$args .= " " . escapeshellarg($paent['range']['from'] . "-" . 
						$paent['range']['to']);
			}
			
			mwexec_bg("/usr/local/sbin/choparp " . $args);
		}
	}
}

function services_dnsupdate_process() {
	global $config, $g;
	
	/* Dynamic DNS updating active? */
	if (isset($config['dnsupdate']['enable'])) {
		
		$wanip = get_current_wan_address();
		
		if ($wanip) {
				
			$keyname = $config['dnsupdate']['keyname'];
			/* trailing dot */
			if (substr($keyname, -1) != ".")
				$keyname .= ".";
			
			$hostname = $config['dnsupdate']['host'];
			/* trailing dot */
			if (substr($hostname, -1) != ".")
				$hostname .= ".";
			
			/* write public key file */
			if ($config['dnsupdate']['keytype'] == "host") {
				$flags = 513;
				$proto = 3;
			} else if ($config['dnsupdate']['keytype'] == "user") {
				$flags = 0;
				$proto = 2;
			} else {
				$flags = 257;
				$proto = 3;
			}
			
			/* write private key file */
			$fd = fopen("{$g['varetc_path']}/K{$keyname}+157+00000.private", "w");
			$privkey .= <<<EOD
Private-key-format: v1.2
Algorithm: 157 (HMAC)
Key: {$config['dnsupdate']['keydata']}

EOD;
			fwrite($fd, $privkey);
			fclose($fd);
			
			$cmd = "/usr/local/bin/dudders";
			if (isset($config['dnsupdate']['usetcp']))
				$cmd .= " -T";
			$cmd .= " -k {$g['varetc_path']}/K{$keyname}+157+00000.private " .
				escapeshellarg($hostname) . " " . escapeshellarg($config['dnsupdate']['ttl']) . " " . 
				escapeshellarg($wanip);
			
			mwexec_bg($cmd);
		}
	}
	
	return 0;
}

function services_wol_send($ipaddr, $subnet, $mac) {
	/* determine broadcast address */
	$bcip = gen_subnet_max($ipaddr, $subnet);
	
	mwexec("/usr/local/bin/wol -i {$bcip} {$mac}");
}

function services_croen_configure() {
	global $config, $g;

	// Default config
	croen_set_default_config();
	// Kill running croen daemon
	killbypid("{$g['varrun_path']}/croen.pid");
	// Kill running minicron daemon
	killbypid("{$g['varrun_path']}/croen_wait.pid");
	// Make sure there is no croen.tmp file present
	if (file_exists($g['tmp_path'].'/croen.tmp')) {
		unlink($g['tmp_path'].'/croen.tmp');
	}
	
	if (isset($config['croen']['enable'])) {
		// Print start message if booting
		if ($g['booting']) {
			echo "Starting scheduler... ";
		}
		
		// Set croen config vars
		$scheduler_path = '/etc/croen_jobs/scheduler';
		$conf_vars = Array(
			'pidfile' 		=> $g['varrun_path'].'/croen.pid',
			'tmpfile' 		=> $g['tmp_path'].'/croen.tmp',
			'call'			=> '/usr/local/bin/php '.$scheduler_path,
			'interval'		=> (isset($config['croen']['interval']) ? $config['croen']['interval'] * 60 : 600));
		
		// Check for a (hopefully) valid time
		if (time() > mktime(0, 0, 0, 1, 1, 2012)) {
			// Open config file
			$fd = fopen($g['varetc_path'].'/croen.conf', 'w');
			// Write config settings
			$conf_first = TRUE;
			foreach ($conf_vars AS $conf_name => $conf_var) {
				fwrite($fd, (!$conf_first ? "\n" : "").$conf_name." = ".$conf_var);
				$conf_first = FALSE;
			}
			// Write jobs
			foreach ($config['croen']['jobset'] AS $jobset_id => $jobset) {
				// Ignore disabled jobsets
				if (!isset($jobset['disabled'])) {
					fwrite($fd, "\njob = ".
						$jobset_id." ".$jobset['repeat'].(isset($jobset['time']) ? " ".$jobset['time'] : "").
						(isset($jobset['date']) ? " ".$jobset['date'] :
						(isset($jobset['weekday']) ? " ".$jobset['weekday'] :
						(isset($jobset['day']) ? " ".$jobset['day'] :
						(isset($jobset['minute']) ? " ".$jobset['minute'] : ""))))
					);
				}
			}
			// Close config file
			fclose($fd);
			
			// Start croen daemon
			mwexec("/usr/local/bin/croen");
		} else {
			// If time is invalid sleep for 60 seconds and try again (minicron)
			mwexec("/usr/local/bin/minicron 60 ".$g['varrun_path']."/croen_wait.pid ".$scheduler_path);
		}
		
		// Print end message if booting
		if ($g['booting']) {
			echo "done\n";
		}
	}
	
	return 0;
}

?>
