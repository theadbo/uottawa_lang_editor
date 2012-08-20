<?php
/**
 * @file
 * Provides replacements for the revisions for the dual-language editor.
 */

/**
 * Form builder for the confirmation form to revert 1-2 versions.
 */
function uottawa_lang_editor_revert_confirm($form, &$form_state, $nodes) {
  // store the nodes we want to save
  $form = array(
    "#node_revision" => $nodes
  );
  $revision_info = array();
  $id = 0;
  // build a list of nodes that we will be reverted, to be displayed
  foreach ($nodes as $node) {
    $id = $node->nid;
    $revision_info[$node->language] = ($node->language == "en" ? t("English") : t("French")) . ": " . format_date($node->revision_timestamp);
  }
  // make the confirmation form and return it
  return confirm_form($form, t("Are you sure you want to revert to the following revision(s)?") .
          "<br />" . implode("<br />", $revision_info), "node/" . $id . "/revisions", "", t("Revert"), t("Cancel"));
}

/**
 * Processes multiple node reverts.
 */
function uottawa_lang_editor_revert_confirm_submit($form, &$state) {
  // retrieve list of nodes
  $nodes = $form["#node_revision"];
  // if we're not updating both nodes
  if (count($nodes) == 1) {
    // check if we're forcing linked revisions.
    if (in_array(variable_get("uottawa_lang_editor_linked_revisions"), array("S", "Y"))) {
      // if we are, get the first node
      $node = reset($nodes);
      // find the current version of the matching node
      if ($node->language == "fr") {
        $nodes[$node->tnid] = node_load($node->tnid);
      }
      else {
        $trans = translation_node_get_translations($node->tnid);
        $nodes[$trans["fr"]->nid] = node_load($trans["fr"]->nid);
      }
    }
  }
  $id = 0;
  $alt_id = 0;
  $fields = array("english_nid" => 0, "french_nid" => 0);
  foreach ($nodes as $node) {
    $alt_id = $node->nid;
    // set up the revision entries
    $node->revision = 1;
    $node->log = str_replace("!timestamp", format_date($node->revision_timestamp), variable_get("uottawa_lang_editor_comment_revert"));
    // save the revision
    node_save($node);
    // make notes of the field values for later
    if ($node->language == "en") {
      $fields["english_nid"] = $node->nid;
      $fields["english_vid"] = $node->vid;
      $id = $node->nid;
    }
    elseif ($node->language = "fr") {
      $fields["french_nid"] = $node->nid;
      $fields["french_vid"] = $node->vid;
    }
    // message to the user
    drupal_set_message(t("@type %title was reverted to the version from %timestamp", array(
          "@type" => node_type_get_type($node)->name,
          "%title" => $node->title,
          "%timestamp" => format_date($node->revision_timestamp))
        ), "status");
  }
  // keep track of the linked revisions here
  if ($fields["english_nid"] > 0 && $fields["french_nid"] > 0) {
    db_insert("uottawa_lang_editor_revision_links")->fields($fields)->execute();
  }
  // get a valid node ID and redirect to the revision page
  if ($id == 0) {
    $id = $alt_id;
  }
  $state["redirect"] = "node/" . $id . "/revisions";
}

/**
 * Confirmation page for deleting multiple revisions.
 */
function uottawa_lang_editor_delete_confirm($form, &$state, $nodes) {
  // store the revisions for later
  $form = array(
    "#node_revision" => $nodes
  );
  $revision_info = array();
  $id = 0;
  // make a list for the user of all the revisions we are deleting
  foreach ($nodes as $node) {
    $id = $node->nid;
    $revision_info[$node->language] = ($node->language == "en" ? t("English") : t("French")) . ": " . format_date($node->revision_timestamp);
  }
  // make a confirmation form
  return confirm_form($form, t("Are you sure you want to delete the following revision(s)?") .
          "<br />" . implode("<br />", $revision_info), "node/" . $id . "/revisions", "", t("Delete"), t("Cancel"));
}

/**
 * Deletes the revisions.
 */
function uottawa_lang_editor_delete_confirm_submit($form, &$state) {
  // load the nodes in
  $nodes = $form["#node_revision"];
  // we need to keep track of these to update the database
  // and to set up the redirect
  $english_nid = 0;
  $french_nid = 0;
  $english_vid = 0;
  $french_vid = 0;
  foreach ($nodes as $node) {
    // track the values we need
    if ($node->language == "en") {
      $english_nid = $node->nid;
      $english_vid = $node->vid;
    }
    elseif ($node->language == "fr") {
      $french_nid = $node->nid;
      $french_vid = $node->vid;
    }
    // delete the revision
    node_revision_delete($node->vid);
    // message for the user
    drupal_set_message(t("Revision from %timestamp of @type %title has been deleted.", array(
          "@type" => node_type_get_type($node)->name,
          "%title" => $node->title,
          "%timestamp" => format_date($node->revision_timestamp))
        ), "status");
  }
  // update the database of revision links to remove this pair
  db_delete("uottawa_lang_editor_revision_links")
      ->condition(db_or()
          ->condition("english_vid", $english_vid)
          ->condition("french_vid", $french_vid)
      )->execute();
  // redirect back
  if ($english_nid == 0) {
    $english_nid = $french_nid;
  }
  $state["redirect"] = "node/" . $english_nid . "/revisions";
}

/**
 * Shows the specified revisions.
 *
 * @param array $nodes
 *   Array of nodes to view
 */
function uottawa_lang_editor_view_revision($nodes) {
  foreach ($nodes as $node) {
    node_tag_new($node);
  }
  return node_view_multiple($nodes, 'full');
}


/**
 * Builds the dual-revision list
 *
 * @param object $node
 *   The node to show revisions for
 *
 * @return array
 *   Array for drupal_render
 */
function uottawa_lang_editor_dual_revision_list($node) {
  // check for a dual-language node
  module_load_include('inc', 'uottawa_lang_editor', 'uottawa_lang_editor.functions');
  if (!uottawa_lang_editor_is_dual_entry($node->type) || $node->tnid < 1) {
    return uottawa_lang_editor_load_original_revisions($node);
  }
  $translations = translation_node_get_translations($node->tnid);
  if (empty($translations['en']) || empty($translations['fr'])) {
    return uottawa_lang_editor_load_original_revisions($node);
  }
  // Set the title
  drupal_set_title(t('Revisions for %title', array('%title' => $node->title)), PASS_THROUGH);

  return drupal_get_form('uottawa_lang_editor_diff_form', node_load($translations['en']->nid));
}

function uottawa_lang_editor_load_original_revisions($node) {
  if (!module_exists('diff')) {
    module_load_include("inc", "node", "node.pages");
    return node_revision_overview($node);
  }
  else {
    module_load_include("inc", "diff", "diff.pages");
    return diff_diffs_overview($node);
  }
}

/**
 * Validation callback for the diff buttons.
 *
 * Make sure we haven't selected the same versions to compare.
 */
function uottawa_lang_editor_diff_validate($form, &$form_state) {
  $old = $form_state["values"]["diff"]["old"];
  $new = $form_state["values"]["diff"]["new"];
  if ($old == $new) {
    form_set_error('diff][new', t('You cannot select the same revision to compare'));
  }
}

/**
 * Submit handler for doing a diff between two versions.
 */
function uottawa_lang_editor_diff_submit($form, &$form_state) {
  $nid = $form_state['values']['nid'];
  $vids = $form_state['values']['diff']['old'] . '-' . $form_state['values']['diff']['new'];
  $url = 'node/' . $nid . '/revisions/' . $vids . '/diff';
  $form_state['redirect'] = $url;
}

/**
 * The revision form page.
 */
function uottawa_lang_editor_diff_form($form, $form_state, $node) {
  // find the french node
  $translations = translation_node_get_translations($node->tnid);
  $french_nid = $translations["fr"]->nid;

  // build the list of revisions
  $ensemble = uottawa_lang_editor_revision_ensemble($node, $translations["fr"]);

  // revert permission
  $revert_permission = FALSE;
  if ((user_access('revert revisions') || user_access('administer nodes')) && node_access('update', $node)) {
    $revert_permission = TRUE;
  }

  // delete permission
  $delete_permission = FALSE;
  if ((user_access('delete revisions') || user_access('administer nodes')) && node_access('delete', $node)) {
    $delete_permission = TRUE;
  }

  $form = array();
  $form["#tree"] = TRUE;
  $form["#theme"] = "uottawa_lang_editor_diff";
  $form["info"] = array();
  $form["fra_info"] = array();

  $form["nid"] = array(
    '#type' => 'value',
    '#value' => $node->nid
  );

  $english_first = TRUE;
  $french_first = TRUE;
  $first_key = "";
  $second_key = "";
  $key_list = array();
  foreach ($ensemble as $entry) {
    $ops = array();
    $en_ops = FALSE;
    $fr_ops = FALSE;
    $key = array();
    if (!empty($entry["en"])) {
      $key[] = $entry["en"]->vid;
      $item = uottawa_lang_editor_build_revision_entry($english_first, $node->nid, $entry["en"], ($english_first ? "revision-current" : ""));
      $form["info"][] = array(
        "#markup" => $item["data"],
        "#attributes" => array(
          'class' => $item["class"]
        )
      );
      if (!$english_first) {
        $en_ops = TRUE;
      }
      else {
        $english_first = FALSE;
      }
    }
    else {
      $form["info"][] = array(
        '#markup' => 'No revision found',
        '#attributes' => array(
          'class' => array('revision-none')
        )
      );
    }
    if (!empty($entry["fr"])) {
      $key[] = $entry["fr"]->vid;
      $item = uottawa_lang_editor_build_revision_entry($french_first, $node->nid, $entry["fr"], ($french_first ? "revision-current" : ""));
      $form["info_fra"][] = array(
        "#markup" => $item["data"],
        "#attributes" => array(
          'class' => $item["class"]
        )
      );
      if (!$french_first) {
        $fr_ops = TRUE;
      }
      else {
        $french_first = FALSE;
      }
    }
    else {
      $form["info_fra"][] = array(
        '#markup' => 'No revision found',
        '#attributes' => array(
          'class' => array('revision-none')
        )
      );
    }
    $fullkey = implode("-", $key);
    if (empty($first_key)) {
      $first_key = $fullkey;
    }
    elseif (empty($second_key)) {
      $second_key = $fullkey;
    }
    $key_list[$fullkey] = '';
    if ($revert_permission || $delete_permission) {
      $revert_ops = array();
      $delete_ops = array();
      if ($en_ops) {
        $revert_ops[] = l(t("English Only"), "node/$node->nid/revisions/" . $entry["en"]->vid . "/revert-dual");
        $delete_ops[] = l(t("English Only"), "node/$node->nid/revisions/" . $entry["en"]->vid . "/delete-dual");
      }
      // french revert
      if ($fr_ops) {
        $revert_ops[] = l(t("French Only"), "node/$node->nid/revisions/" . $entry["fr"]->vid . "/revert-dual");
        $delete_ops[] = l(t("French Only"), "node/$node->nid/revisions/" . $entry["fr"]->vid . "/delete-dual");
      }
      // revert both
      if ($en_ops && $fr_ops) {
        $revert_ops[] = l(t("Both"), "node/$node->nid/revisions/" . $entry["en"]->vid . "-" . $entry["fr"]->vid . "/revert-dual");
        $delete_ops[] = l(t("Both"), "node/$node->nid/revisions/" . $entry["en"]->vid . "-" . $entry["fr"]->vid . "/delete-dual");
      }
      $ops = array(0 => array(), 1 => array());
      if ($revert_permission) {
        // build ops
        $ops[0] = array("#markup" => implode("<br />", $revert_ops));
      }
      if ($delete_permission) {
        // build ops
        $ops[1] = array("#markup" => implode("<br />", $delete_ops));
      }
      $form["operations"][] = $ops;
    }
  }
  $form['diff']['old'] = array(
    '#type' => 'radios',
    '#options' => $key_list,
    '#default_value' => $first_key,
  );
  $form['diff']['new'] = array(
    '#type' => 'radios',
    '#options' => $key_list,
    '#default_value' => $second_key,
  );
  $form["submit"] = array(
    "#type" => "submit",
    "#value" => t("Compare"),
    '#submit' => array('uottawa_lang_editor_diff_submit'),
    '#validate' => array('uottawa_lang_editor_diff_validate'),
  );
  return $form;
}

/**
 * Assemble the list of revisions for two nodes (English and French)
 *
 * @param object $node
 *   The English node
 *
 * @param object $french_node
 *   The French node
 */
function uottawa_lang_editor_revision_ensemble($node, $french_node) {
  $ensemble = array();
  // english revisions
  $revisions = node_revision_list($node);
  // french revisions
  $french_revisions = node_revision_list($french_node);
  // build an array of English revs with the vid as keys
  $en_revs = array();
  foreach ($revisions as $rev) {
    $en_revs[$rev->vid] = $rev;
  }
  // build an array of French revs with the vid as keys
  $fr_revs = array();
  foreach ($french_revisions as $rev) {
    $fr_revs[$rev->vid] = $rev;
  }
  // look for all the linked revisions
  $rs = db_query("SELECT * FROM {uottawa_lang_editor_revision_links} WHERE english_nid = :enid AND french_nid = :fnid", array(
    ":enid" => $node->nid,
    ":fnid" => $french_node->nid
      ));
  foreach ($rs as $link) {
    // if the english and french vids are set (ie exist for these nodes)
    if (isset($fr_revs[$link->french_vid]) && isset($en_revs[$link->english_vid])) {
      // load the matched pairs in
      $ensemble[] = array(
        "en" => $en_revs[$link->english_vid],
        "fr" => $fr_revs[$link->french_vid]
      );
      // remove them from the revision list so that they are not added later
      unset($en_revs[$link->english_vid]);
      unset($fr_revs[$link->french_vid]);
    }
  }
  // add in the single english revisions
  foreach ($en_revs as $revision) {
    $ensemble[] = array("en" => $revision);
  }
  // add in the single french revisions
  foreach ($fr_revs as $revision) {
    $ensemble[] = array("fr" => $revision);
  }
  // sort revisions by date
  usort($ensemble, "uottawa_lang_editor_revision_sort");
  return $ensemble;
}

/**
 * Sort revisions by English vid, then French vid
 *
 * @param array $a
 *   First set of versions
 *
 * @param array $b
 *   Second set of versions
 *
 * @return int
 *   Sorted index
 */
function uottawa_lang_editor_revision_sort($a, $b) {
  $a_vid = isset($a["en"]) ? $a["en"]->vid : $a["fr"]->vid;
  $b_vid = isset($b["en"]) ? $b["en"]->vid : $b["fr"]->vid;
  if ($a_vid == $b_vid) {
    return 0;
  }
  return ($a_vid < $b_vid) ? 1 : -1;
}

/**
 * Constructs a log entry for a revision.
 *
 * @param boolean $first
 *   Is this the first entry?
 *
 * @param int $nid
 *   The nid for the node
 *
 * @param object $revision
 *   The revision we're generating an entry for
 *
 * @param string|array $class
 *   A single class name or an array of classes
 *
 * @return string
 *   The log entry (HTML code)
 */
function uottawa_lang_editor_build_revision_entry($first, $nid, $revision, $class = "") {
  // Add CSS to help display these
  drupal_add_css(drupal_get_path("module", "uottawa_lang_editor") . "/uottawa_lang_editor_revisions.css");
  $info = array("class" => array(), "data" => "");
  if ($first) {
    $info["data"] = t('!date by !username', array(
      // the first entry links to the node
      '!date' => l(format_date($revision->timestamp, 'short'), 'node/' . $nid),
      '!username' => theme("username", array("account" => $revision))
        ));
  }
  else {
    $info["data"] = t('!date by !username', array(
      // 2+ entries link to the revisions
      '!date' => l(format_date($revision->timestamp, 'short'), 'node/' . $nid . "/revisions/" . $revision->vid . "/view-dual"),
      '!username' => theme("username", array("account" => $revision))
        ));
  }
  // add classes to the data
  if (!empty($class)) {
    if (is_array($class)) {
      $info["class"] = $class;
    }
    else {
      $info["class"] = array($class);
    }
  }
  // check what's in the log
  if ($revision->log != '') {
    $log_text = $revision->log;
    // remove creation prefix and add class
    if (strpos($revision->log, "LAC:: ") === 0) {
      $info["class"][] = "revision-creation";
      $log_text = drupal_substr($log_text, 6);
    }
    // remove minor change prefix and add class
    elseif (strpos($revision->log, "LAM:: ") === 0) {
      $info["class"][] = "revision-minor";
      $log_text = drupal_substr($log_text, 6);
    }
    // remove no changes prefix and add class
    elseif (strpos($revision->log, "LAN:: ") === 0) {
      $info["class"][] = "revision-no-changes";
      $log_text = drupal_substr($log_text, 6);
    }
    // filter data
    $info["data"] .= '<p class="revision-log">' . filter_xss($log_text) . '</p>';
  }
  // return row information
  return $info;
}