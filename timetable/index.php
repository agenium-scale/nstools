<?php
/* Copyright (C) 2001-2005 Rodolphe Quiedeville <rodolphe@quiedeville.org>
 * Copyright (C) 2004-2015 Laurent Destailleur  <eldy@users.sourceforge.net>
 * Copyright (C) 2005-2012 Regis Houssin        <regis.houssin@inodbox.com>
 * Copyright (C) 2015      Jean-François Ferry  <jfefe@aternatik.fr>
 * Copyright (C) 2019      Guillaume Quintin    <guillaume.quintin@agenium.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/**
 *  \file       htdocs/mymodule/index.php
 *  \ingroup    TimeTable
 *  \brief      Home page of TimeTable top menu
 */

// Load Dolibarr environment
$res=0;
// Try main.inc.php into web root known defined into CONTEXT_DOCUMENT_ROOT (not always defined)
if (! $res && ! empty($_SERVER["CONTEXT_DOCUMENT_ROOT"])) $res=@include $_SERVER["CONTEXT_DOCUMENT_ROOT"]."/main.inc.php";
// Try main.inc.php into web root detected using web root calculated from SCRIPT_FILENAME
$tmp=empty($_SERVER['SCRIPT_FILENAME'])?'':$_SERVER['SCRIPT_FILENAME'];$tmp2=realpath(__FILE__); $i=strlen($tmp)-1; $j=strlen($tmp2)-1;
while($i > 0 && $j > 0 && isset($tmp[$i]) && isset($tmp2[$j]) && $tmp[$i]==$tmp2[$j]) { $i--; $j--; }
if (! $res && $i > 0 && file_exists(substr($tmp, 0, ($i+1))."/main.inc.php")) $res=@include substr($tmp, 0, ($i+1))."/main.inc.php";
if (! $res && $i > 0 && file_exists(dirname(substr($tmp, 0, ($i+1)))."/main.inc.php")) $res=@include dirname(substr($tmp, 0, ($i+1)))."/main.inc.php";
// Try main.inc.php using relative path
if (! $res && file_exists("../main.inc.php")) $res=@include "../main.inc.php";
if (! $res && file_exists("../../main.inc.php")) $res=@include "../../main.inc.php";
if (! $res && file_exists("../../../main.inc.php")) $res=@include "../../../main.inc.php";
if (! $res) die("Include of main fails");

require_once DOL_DOCUMENT_ROOT.'/core/class/html.formfile.class.php';

// Load translation files required by the page
$langs->loadLangs(array("timetable@timetable"));

// Securite acces client
if (!$user->rights->timetable->read) {
  accessforbidden();
}

// ----------------------------------------------------------------------------
// Initialize some variables

$form = new Form($db);
$now = dol_now();
$post_begin = GETPOST('timetable_begindate_');
$post_end = GETPOST('timetable_enddate_');
$begin = dol_mktime(0, 0, 0, GETPOST('timetable_begindate_month'),
                    GETPOST('timetable_begindate_day'),
                    GETPOST('timetable_begindate_year'));
$end = dol_mktime(0, 0, 0, GETPOST('timetable_enddate_month'),
                  GETPOST('timetable_enddate_day'),
                  GETPOST('timetable_enddate_year'));
$last_month_begin = date("Y-m-d", $post_begin === '' ?
                                  strtotime("first day of previous month") :
                                  $begin);
$last_month_end = date("Y-m-d", $post_end === '' ?
                                strtotime("last day of previous month") :
                                $end);

// ----------------------------------------------------------------------------
// Build HTML

// Header
llxHeader("", $langs->trans("timetable"));

// Body
print load_fiche_titre($langs->trans("timetable"), '', '');
print '<p>'.$langs->trans('explanations').'</p>';
print '<p>'.$langs->trans('default').'</p>';

dol_fiche_head();

print '<form method="POST" action="'.$_SERVER['PHP_SELF'].'" name="timetable_generate">';
print '<input type="hidden" name="token" value="'.$_SESSION['newtoken'].'" />';
print '<input type="hidden" name="action" value="generate" />';
print '<table class="border" width="100%">';
print '<tbody>';
print '<tr>';
print '<td class="titlefield fieldrequired">'.$langs->trans("begindate").'</td>';
print '<td>'.$form->selectDate($last_month_begin, 'timetable_begindate_', 0, 0, 0, '', 1, 1).'</td>';
print '</tr><tr>';
print '<td class="titlefield fieldrequired">'.$langs->trans("enddate").'</td>';
print '<td>'.$form->selectDate($last_month_end, 'timetable_enddate_', 0, 0, 0, '', 1, 1).'</td>';
print '</tr></tbody></table>';
print '<div class="center">';
print '<input type="submit" value="'.$langs->trans("generate").'" name="bouton" class="button">';
print '</div>';
print '</form>';

dol_fiche_end();

// When we have our dates, then generates the file
if (GETPOST('action', 'alpha') === 'generate') {
  if (trim($post_begin) === '' || trim($post_end) === '') {
    print '<p><em>'.$langs->trans('error_misformed_date').'</em></p>';
    goto print_footer;
  }

  if ($begin > $end) {
    print '<p><em>'.$langs->trans('error_end_begin').'</em></p>';
    goto print_footer;
  }

  // compute timetable now
  // get all times (a request to the DB seems necessary cf.
  // projet/tasks/time.php) and users and project
  $sql =
   "SELECT t.task_date, pt.label, u.login, u.firstname, u.lastname, p.title"
  ." FROM (((".MAIN_DB_PREFIX."projet_task_time as t"
  ." INNER JOIN ".MAIN_DB_PREFIX."projet_task as pt ON t.fk_task = pt.rowid)"
  ." INNER JOIN ".MAIN_DB_PREFIX."projet as p ON pt.fk_projet = p.rowid)"
  ." INNER JOIN ".MAIN_DB_PREFIX."user as u ON t.fk_user = u.rowid)"
  ." WHERE t.task_date >= '".date("Y-m-d", $begin)
  ."' AND t.task_date <= '".date("Y-m-d", $end)."';";
  $resql = $db->query($sql);
  if (!$resql) {
    dol_print_error($db);
    exit;
  }

  // format for handling date (default to french)
  // this will need to be put as a settings for further developements
  $date_format = "d/m/Y";

  // get informations from database
  $csv = array();
  $users = array();
  for ($i = 0; $i < $db->num_rows($resql); $i++) {
    $row = $db->fetch_object($resql);
    $date = date("Y/m/d", strtotime($row->task_date));
    $login = $row->login;
    $task = $row->label;
    $project = strpos($row->title, "meta") === FALSE ?
               $row->title : $row->label;
    $firstname = $row->firstname;
    $lastname = $row->lastname;

    // fill $csv
    if (array_key_exists($date, $csv) === FALSE) {
      $csv[$date] = array();
    }
    $csv[$date][$login] = $project;

    // fill $users
    if (array_key_exists($login, $users) === FALSE) {
      $users[$login] = $lastname." ".$firstname;
    }
  }
  $db->free($resql);

  // sort users by values if any otherwise exit
  if (count($users) == 0) {
    print("<p><em>".$langs->trans('error_no_user')."</em></p>");
    goto print_footer;
  }
  asort($users);
  $empty_row = join(", ", array_fill(0, count($users), '""'));

  // build the array of all days
  $curr = date("Y/m/d", $begin);
  $day_of_week = date("w", strtotime($curr));
  if ($day_of_week != "0" && $day_of_week != "6") {
    $dates = array($curr);
  } else {
    $dates = array();
  }
  do {
    $curr = date("Y/m/d", strtotime($curr." +1 day"));
    $day_of_week = date("w", strtotime($curr));
    if ($day_of_week != "0" && $day_of_week != "6") {
      array_push($dates, $curr);
    }
  } while($curr != date("Y/m/d", $end));

  // generate filename
  $start_month = date("F", $begin);
  $end_month = date("F", $end);
  if ($start_month == $end_month) {
    $filename = "timetable-".$start_month.".csv";
  } else {
    $filename = "timetable-".$start_month."-to-".$end_month.".csv";
  }

  // create CSV as a string in memory
  $buf = '""';
  foreach ($users as $u) {
    $buf .= ", \"".$u."\"";
  }
  $buf .= "\n";
  for ($i = 0; $i < count($dates); $i++) {
    $buf .= '"'.date($date_format, strtotime($dates[$i])).'"';
    if (array_key_exists($dates[$i], $csv)) {
      foreach ($users as $login => $u) {
        $buf .= ", ";
        if (array_key_exists($login, $csv[$dates[$i]])) {
          $buf .= '"'.$csv[$dates[$i]][$login].'"';
        } else {
          $buf .= '""';
        }
      }
    } else {
      $buf .= ", ".$empty_row;
    }
    if ($i < count($dates) - 1) {
      $buf .= "\n";
    }
  }
  //print("<pre><code>".$buf."</code></pre>");

  // write the file
  $dir = DOL_DATA_ROOT."/timetable/temp";
  if (dol_mkdir($dir) < 0 ||
      file_put_contents($dir."/".$filename, $buf) === FALSE) {
    print("<p><em>".$langs->trans('error_write_file')."</em></p>");
    goto print_footer;
  } else {
    print("<p>".$langs->trans('download_link')."</p>");
    // taken from htdocs/core/class/html.formfile.class.php, lines 827-833
    $documenturl = DOL_URL_ROOT.'/document.php';
    if (isset($conf->global->DOL_URL_ROOT_DOCUMENT_PHP)) {
      $documenturl = $conf->global->DOL_URL_ROOT_DOCUMENT_PHP;
    }
    $url = $documenturl.'?modulepart=timetable&file='.
           urlencode('temp/'.$filename);
    print('<p><a href="'.$url.'">'.$filename."</a></p>");
  }
}

// Footer
print_footer:
llxFooter();
$db->close();
