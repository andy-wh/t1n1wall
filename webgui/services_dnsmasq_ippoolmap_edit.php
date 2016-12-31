#!/usr/local/bin/php
<?php 
/*
	$Id: services_dnsmasq_ippoolmap_edit.php 411 2016-12-30 12:58:55Z awhite $
	part of t1n1wall (http://t1n1wall.com)
	
	Copyright (C) 2015-2017 Andrew White <andy@t1n1wall.com>.
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

$pgtitle = array("Services", "DNS forwarder", "Edit IPpool Map");
require("guiconfig.inc");

if (!is_array($config['dnsmasq']['ippoolmaps'])) {
	$config['dnsmasq']['ippoolmaps'] = array();
}

ippoolmaps_sort();
$a_ippoolmaps = &$config['dnsmasq']['ippoolmaps'];

ippools_sort();
$a_ippools = &$config['ippools']['ippool'];

$id = $_GET['id'];
if (isset($_POST['id']))
	$id = $_POST['id'];

if (isset($id) && $a_ippoolmaps[$id]) {
	$pconfig['fqdn'] = $a_ippoolmaps[$id]['fqdn'];
	$pconfig['poolid'] = $a_ippoolmaps[$id]['poolid'];
	$pconfig['descr'] = $a_ippoolmaps[$id]['descr'];
}

if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;

	/* input validation */
	$reqdfields = explode(" ", "domain poolid");
	$reqdfieldsn = explode(",", "Domain,IPpool");
	
	do_input_validation($_POST, $reqdfields, $reqdfieldsn, &$input_errors);
	
	if (($_POST['domain'] && !is_domain($_POST['domain']))) {
		$input_errors[] = "A valid domain must be specified.";
	}
	if (($_POST['ippool'] && !is_ippool($_POST['ippool']))) {
		$input_errors[] = "A valid IPpool must be specified.";
	}


	if (!$input_errors) {
		$poolmap = array();
		$poolmap['fqdn'] = $_POST['domain'];
		$poolmap['poolid'] = $_POST['poolid'];
		$poolmap['descr'] = $_POST['descr'];

		if (isset($id) && $a_ippoolmaps[$id])
			$a_ippoolmaps[$id] = $poolmap;
		else
			$a_ippoolmaps[] = $poolmap;
		
		touch($d_dnsmasqdirty_path);
		
		write_config();
		
		header("Location: services_dnsmasq.php");
		exit;
	}
}
?>
<?php include("fbegin.inc"); ?>
<?php if ($input_errors) print_input_errors($input_errors); ?>
            <form action="services_dnsmasq_ippoolmap_edit.php" method="post" name="iform" id="iform">
              <table width="100%" border="0" cellpadding="6" cellspacing="0" summary="content pane">
				<tr>
                  <td width="22%" valign="top" class="vncellreq">Domain</td>
                  <td width="78%" class="vtable"> 
                    <?=$mandfldhtml;?><input name="domain" type="text" class="formfld" id="domain" size="40" value="<?=htmlspecialchars($pconfig['fqdn']);?>">
                    <br> <span class="vexpl">Domain to resolve ips for pools<br>
                    e.g. <em>test</em></span></td>
                </tr>
				<tr>
                  <td width="22%" valign="top" class="vncellreq">IPpool</td>
                  <td width="78%" class="vtable"> 
                    <?=$mandfldhtml;?><select name="poolid" type="text" class="formfld" id="poolid" >

                  <?php $i = 0; foreach ($a_ippools as $ippool): ?>
                  <option value="<?=$ippool['poolid'];?>" <?php if ($ippool['poolid'] == $pconfig['poolid']) echo "selected"; ?>><?=$ippool['name'];?></option>
				  <?php endforeach; ?>

                    <br><span class="vexpl">IP pool to insert ip addresses into
                    </span></td>
                </tr>
				<tr>
                  <td width="22%" valign="top" class="vncell">Description</td>
                  <td width="78%" class="vtable"> 
                    <input name="descr" type="text" class="formfld" id="descr" size="40" value="<?=htmlspecialchars($pconfig['descr']);?>">
                    <br> <span class="vexpl">You may enter a description here
                    for your reference (not parsed).</span></td>
                </tr>
                <tr>
                  <td width="22%" valign="top">&nbsp;</td>
                  <td width="78%"> 
                    <input name="Submit" type="submit" class="formbtn" value="Save">
                    <?php if (isset($id) && $a_ippoolmaps[$id]): ?>
                    <input name="id" type="hidden" value="<?=htmlspecialchars($id);?>">
                    <?php endif; ?>
                  </td>
                </tr>
              </table>
</form>
<?php include("fend.inc"); ?>
