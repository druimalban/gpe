<?php

function menu_entry($page, $title, $link,$thispage)
{
  if ($page == $thispage) {
    echo "<b>$title</b>";
  } else {
    echo "<a href='$link'>$title</a>";
  }
}

?>

<div class="menu">
<small>

<?php menu_entry("home", "Home/News", "/", $pagename); ?>
 &middot;
<?php menu_entry("projects", "Projects", "/projects/", $pagename); ?>
 &middot;
<?php menu_entry("documentation", "Documentation", "/documentation.phtml", $pagename); ?>
 &middot;
<A href="/gallery/gallery/">Screenshots</A>
 &middot;
<?php menu_entry("download", "Download", "/download.phtml", $pagename); ?>
 &middot;
<?php menu_entry("contact", "Mailing list/IRC", "/contact.phtml", $pagename); ?>
 &middot;
<A href="http://handhelds.org/bugzilla/">Bugs</A>
 &middot;
<?php menu_entry("cvs", "CVS", "/cvs.phtml", $pagename); ?>
 &middot;
<?php menu_entry("linkres", "Links/Resources", "/linkres.phtml", $pagename); ?>

</small>
</div>
