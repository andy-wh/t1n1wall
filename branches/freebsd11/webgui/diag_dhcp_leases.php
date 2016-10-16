#!/usr/local/bin/php
<?php 
/*
	$Id: diag_dhcp_leases.php 411 2010-11-12 12:58:55Z mkasper $
	part of m0n0wall (http://m0n0.ch/wall)
	
	Copyright (C) 2003-2005 Björn Pålsson <bjorn@networksab.com> and Manuel Kasper <mk@neon1.net>.
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

$pgtitle = array("Diagnostics", "DHCP leases");

require("guiconfig.inc");
?>
<?php include("fbegin.inc"); ?>
<?php

flush();

function leasecmp($a, $b) {
	return strcmp($a[$_GET['order']], $b[$_GET['order']]);
}

function adjust_gmt($dt) {
	return strftime("%Y/%m/%d %H:%M:%S", $dt);
}

$fp = @fopen("{$g['vardb_path']}/dnsmasq.dhcpd.leases","r");

if ($fp):


$i=0;
while ($line = fgets($fp)) {
	#1451696413 08:00:27:61:7f:f7 192.168.1.101 SunLamb 01:08:00:27:61:7f:f7
	$leases[$i] = array();
	list($leases[$i]['end'],$leases[$i]['mac'],$leases[$i]['ip'],$leases[$i]['hostname'],$leases[$i]['cid']) = explode(" ",$line);
	$i++;
}

fclose($fp);

if ($_GET['order'])
	usort($leases, "leasecmp");
?>
<table width="100%" border="0" cellpadding="0" cellspacing="0" summary="tab pane">
  <tr>
    <td class="listhdrr"><a href="?all=<?=htmlspecialchars($_GET['all']);?>&amp;order=ip">IP address</a></td>
    <td class="listhdrr"><a href="?all=<?=htmlspecialchars($_GET['all']);?>&amp;order=mac">MAC address</a></td>
    <td class="listhdrr"><a href="?all=<?=htmlspecialchars($_GET['all']);?>&amp;order=hostname">Hostname</a></td>
    <td class="listhdrr"><a href="?all=<?=htmlspecialchars($_GET['all']);?>&amp;order=clid">ClientID</a></td>
    <td class="listhdr"><a href="?all=<?=htmlspecialchars($_GET['all']);?>&amp;order=end">End</a></td>
    <td class="list"></td>
	</tr>
<?php
foreach ($leases as $data) {
		$lip = ip2long32($data['ip']);
		foreach ($config['dhcpd'] as $dhcpif => $dhcpifconf) {
			if (($lip >= ip2long32($dhcpifconf['range']['from'])) && ($lip <= ip2long32($dhcpifconf['range']['to']))) {
				$data['if'] = $dhcpif;
				break;
			}
		}
		echo "<tr>\n";
		echo "<td class=\"listlr\">{$data['ip']}&nbsp;</td>\n";
		echo "<td class=\"listr\">{$data['mac']}&nbsp;</td>\n";
		echo "<td class=\"listr\">" . htmlentities($data['hostname']) . "&nbsp;</td>\n";
		echo "<td class=\"listr\">" . htmlentities($data['cid']) . "&nbsp;</td>\n";
		echo "<td class=\"listr\">" . adjust_gmt($data['end']) . "&nbsp;</td>\n";
		echo "<td class=\"list\" valign=\"middle\"><a href=\"services_dhcp_edit.php?if={$data['if']}&amp;mac={$data['mac']}\"><img src=\"plus.png\" width=\"17\" height=\"17\" border=\"0\" title=\"add a static mapping for this MAC address\" alt=\"add a static mapping for this MAC address\"></a></td>\n";
		echo "</tr>\n";
}
?>
</table>
<br>
<form action="diag_dhcp_leases.php" method="GET">
<input type="hidden" name="order" value="<?=htmlspecialchars($_GET['order']);?>">
</form>
<?php else: ?>
<strong>No leases file found. Is the DHCP server active?</strong>
<?php endif; ?>
<?php include("fend.inc"); ?>
