<?php

/* Reminder: always indent with 4 spaces (no tabs). */
// +---------------------------------------------------------------------------+
// | Geeklog 1.3                                                               |
// +---------------------------------------------------------------------------+
// | index.php                                                                 |
// | Geeklog homepage.                                                         |
// |                                                                           |
// +---------------------------------------------------------------------------+
// | Copyright (C) 2000,2001 by the following authors:                         |
// |                                                                           |
// | Authors: Tony Bibbs       - tony@tonybibbs.com                            |
// |          Mark Limburg     - mlimburg@users.sourceforge.net                |
// |          Jason Wittenburg - jwhitten@securitygeeks.com                    |
// +---------------------------------------------------------------------------+
// |                                                                           |
// | This program is free software; you can redistribute it and/or             |
// | modify it under the terms of the GNU General Public License               |
// | as published by the Free Software Foundation; either version 2            |
// | of the License, or (at your option) any later version.                    |
// |                                                                           |
// | This program is distributed in the hope that it will be useful,           |
// | but WITHOUT ANY WARRANTY; without even the implied warranty of            |
// | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             |
// | GNU General Public License for more details.                              |
// |                                                                           |
// | You should have received a copy of the GNU General Public License         |
// | along with this program; if not, write to the Free Software Foundation,   |
// | Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.           |
// |                                                                           |
// +---------------------------------------------------------------------------+
//
// $Id$

$limit = 10;
$topic = "GPE";

$no_sessions_please = 1;

require_once('/home/httpd/html/geeklog/lib-common.php');

$display = '';

$maxstories = 0;

if (isset ($HTTP_GET_VARS['page'])) {
    $page = $HTTP_GET_VARS['page'];
}

if (empty($page)) {
    // If no page sent then assume the first.
    $page = 1;
}

// Geeklog now allows for articles to be published in the future.  Because of
// this, we need to check to see if we need to rebuild the RDF file in the case
// that any such articles have now been published
//  COM_rdfUpToDateCheck();

// For similar reasons, we need to see if there are currently two featured
// articles.  Can only have one but you can have one current featured article
// and one for the future...this check will set the latest one as featured
// solely
// COM_featuredCheck();

$sql = "FROM {$_TABLES['stories']} WHERE (date <= NOW()) AND (draft_flag = 0)";

// if a topic was provided only select those stories.
$sql .= " AND tid = '$topic' ";

$groupsql = " (perm_anon >= 2) ";

$sql .= " AND" . $groupsql;

$offset = ($page - 1) * $limit;
$sql .= "ORDER BY featured DESC, date DESC";

$result = DB_query ("SELECT *,unix_timestamp(date) AS day " . $sql
        . " LIMIT $offset, $limit");
$nrows = DB_numRows($result);

$data = DB_query("SELECT count(*) AS count " . $sql);
$D = DB_fetchArray($data);
$num_pages = ceil($D['count'] / $limit);

if ($nrows > 0) {
    for ($x = 1; $x <= $nrows; $x++) {
        $A = DB_fetchArray($result);
        if ($A['featured'] == 1) {
            $feature = 'true';
        } elseif (($x == 1) && ($_CONF['showfirstasfeatured'] == 1)) {
            $feature = 'true';
            $A['featured'] = 1;
        }
        $display .= COM_article($A,'y');
    }

    // Print Google-like paging navigation
    if (empty($topic)) {
        $base_url = $_CONF['site_url'] . '/index.php';
        if ($newstories) {
            $base_url .= '?display=new';
        }
    } else {
        $base_url = $_CONF['site_url'] . '/index.php?topic=' . $topic;
    }
    $display .= COM_printPageNavigation($base_url,$page, $num_pages);
}

// Output page 
echo $display;

?>
