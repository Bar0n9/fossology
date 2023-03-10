<?php
/*
 SPDX-FileCopyrightText: © 2014-2015 Siemens AG

 SPDX-License-Identifier: GPL-2.0-only
*/

use Fossology\Lib\Dao\LicenseDao;
use Fossology\Lib\Dao\UploadDao;
use Fossology\Lib\Dao\UploadPermissionDao;
use Fossology\Lib\Data\AgentRef;
use Fossology\Lib\Data\LicenseRef;
use Fossology\Lib\Db\DbManager;
use Fossology\Lib\Test\TestPgDb;
use Monolog\Logger;

if (!function_exists('Traceback_uri'))
{
  function Traceback_uri(){
    return 'Traceback_uri_if_desired';
  }
}

class NinkaScheduledTest extends \PHPUnit\Framework\TestCase
{
  /** @var TestPgDb */
  private $testDb;
  /** @var DbManager */
  private $dbManager;
  /** @var LicenseDao */
  private $licenseDao;
  /** @var UploadDao */
  private $uploadDao;
  /** @var UploadPermissionDao */
  private $uploadPermDao;

  protected function setUp() : void
  {
    $this->testDb = new TestPgDb("ninkaSched".time());
    $this->dbManager = $this->testDb->getDbManager();

    $this->licenseDao = new LicenseDao($this->dbManager);
    $logger = new Logger("NinkaSchedulerTest");
    $this->uploadPermDao = \Mockery::mock(UploadPermissionDao::class);
    $this->uploadDao = new UploadDao($this->dbManager, $logger, $this->uploadPermDao);
  }
  
  protected function tearDown() : void
  {
    $this->testDb->fullDestruct();
    $this->testDb = null;
    $this->dbManager = null;
    $this->licenseDao = null;
  }

  private function runNinka($uploadId)
  {
    $sysConf = $this->testDb->getFossSysConf();

    $agentName = "ninka";

    $agentDir = dirname(dirname(__DIR__));
    $execDir = "$agentDir/agent";
    system("install -D $agentDir/VERSION $sysConf/mods-enabled/$agentName/VERSION");

    $pipeFd = popen("echo $uploadId | $execDir/$agentName -c $sysConf --scheduler_start", "r");
    $this->assertTrue($pipeFd !== false, 'running ninka failed');

    $output = "";
    while (($buffer = fgets($pipeFd, 4096)) !== false) {
      $output .= $buffer;
    }
    $retCode = pclose($pipeFd);

    unlink("$sysConf/mods-enabled/$agentName/VERSION");
    rmdir("$sysConf/mods-enabled/$agentName");
    rmdir("$sysConf/mods-enabled");

    return array($output,$retCode);
  }

  private function setUpRepo()
  {
    $sysConf = $this->testDb->getFossSysConf();

    $confFile = $sysConf."/fossology.conf";
    system("touch ".$confFile);
    $config = "[FOSSOLOGY]\ndepth = 0\npath = $sysConf/repo\n";
    file_put_contents($confFile, $config);

    $testRepoDir = dirname(dirname(dirname(__DIR__)))."/lib/php/Test/";
    system("cp -a $testRepoDir/repo $sysConf/");
  }

  private function rmRepo()
  {
    $sysConf = $this->testDb->getFossSysConf();
    system("rm $sysConf/repo -rf");

    unlink($sysConf."/fossology.conf");
  }

  private function setUpTables()
  {
    $this->testDb->createPlainTables(array('upload','uploadtree','license_ref','license_file','highlight','agent','pfile','ars_master'),false);
    $this->testDb->createSequences(array('agent_agent_pk_seq','pfile_pfile_pk_seq','upload_upload_pk_seq','nomos_ars_ars_pk_seq','license_file_fl_pk_seq','license_ref_rf_pk_seq'),false);
    $this->testDb->createViews(array('license_file_ref'),false);
    $this->testDb->createConstraints(array('agent_pkey','pfile_pkey','upload_pkey_idx','FileLicense_pkey','rf_pkpk'),false);
    $this->testDb->alterTables(array('agent','pfile','upload','ars_master','license_file','highlight','license_ref'),false);
    $this->testDb->createInheritedTables();
    $this->testDb->insertData(array('pfile','upload','uploadtree_a'), false);
    $this->testDb->insertData_license_ref();
    $this->testDb->resetSequenceAsMaxOf('license_ref_rf_pk_seq', 'license_ref', 'rf_pk');
  }

  /** @group Functional */
  public function testRun()
  {
    $this->setUpTables();
    $this->setUpRepo();

    list($output,$retCode) = $this->runNinka($uploadId=1);

    $this->rmRepo();

    $this->assertEquals($retCode, 0, 'ninka failed: '.$output);

    $bounds = $this->uploadDao->getParentItemBounds($uploadId);
    $matches = $this->licenseDao->getAgentFileLicenseMatches($bounds);

    $this->assertEquals($expected=6, count($matches));

    foreach($matches as $licenseMatch) {
      /** @var LicenseRef */
      $matchedLicense = $licenseMatch->getLicenseRef();

      switch ($licenseMatch->getFileId()) {
        case 7:
        case 4:
          $expectedLicense = "GPL-3.0+";
          break;
        case 3:
          $expectedLicense = "UnclassifiedLicense";
          break;
        default:
          $expectedLicense = "No_license_found";
            break;
      }

      $this->assertEquals($expectedLicense, $matchedLicense->getShortName(), "unexpected license for fileId ".$licenseMatch->getFileId());

      /** @var AgentRef */
      $agentRef = $licenseMatch->getAgentRef();
      $this->assertEquals($agentRef->getAgentName(), "ninka");
    }
  }

}
