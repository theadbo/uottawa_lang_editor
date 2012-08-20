<?php

/**
 * @file
 * Methods that will replace the admin/content page with a dual-language
 * version of that page.
 */

/**
 * Override of node_admin_content().
 *
 * Tweaked to link into my custom methods for the forms.
 */
function uottawa_lang_editor_content_admin_form($form, &$form_state) {
  if (isset($form_state['values']['operation']) &&
      $form_state['values']['operation'] == 'delete') {
    $form = node_multiple_delete_confirm($form, $form_state,
        array_filter($form_state['values']['nodes']));
    $form['#submit'] =
        array('uottawa_lang_editor_content_admin_delete_form_submit');
    return $form;
  }
  $form['filter'] = uottawa_lang_editor_filter_form();
  $form['#submit'][] = 'uottawa_lang_editor_filter_form_submit';
  $form['admin'] = uottawa_lang_editor_admin_nodes();

  return $form;
}

/**
 * Override of node_multiple_delete_confirm_submit().
 *
 * Tweaked to make sure the file is included on submit.
 */
function uottawa_lang_editor_content_admin_delete_form_submit($form, &$form_state) {
  module_load_include('inc', 'node', 'node.admin');
  return node_multiple_delete_confirm_submit($form, $form_state);
}

/**
 * Submit handler for uottawa_lang_editor_filter_form().
 *
 * Tweaked from node_filter_form_submit() to include support for keeping the
 * filters as drop-downs.
 */
function uottawa_lang_editor_filter_form_submit($form, &$form_state) {
  $filters = node_filters();
  switch ($form_state['values']['op']) {
    case t('Filter'):
    case t('Refine'):
      $_SESSION['node_overview_filter'] = array();
      // Apply every filter that has a choice selected other than 'any'.
      foreach ($filters as $filter => $options) {
        if (isset($form_state['values'][$filter]) &&
            $form_state['values'][$filter] != '[any]') {
          // Flatten the options array to accommodate hierarchical/nested
          // options.
          $flat_options = form_options_flatten($filters[$filter]['options']);
          // Only accept valid selections offered on the dropdown,
          // block bad input.
          if (isset($flat_options[$form_state['values'][$filter]])) {
            $_SESSION['node_overview_filter'][] = array($filter,
              $form_state['values'][$filter]);
          }
        }
      }
      break;
    case t('Undo'):
      array_pop($_SESSION['node_overview_filter']);
      break;
    case t('Reset'):
      $_SESSION['node_overview_filter'] = array();
      break;
  }
}

/**
 * Override of node_filter_form().
 *
 * Tweaked to provide drop-downs for already filtered on options.
 */
function uottawa_lang_editor_filter_form() {
  $session = isset($_SESSION['node_overview_filter']) ?
      $_SESSION['node_overview_filter'] : array();
  module_load_include('inc', 'node', 'node.admin');
  $filters = node_filters();

  $i = 0;
  $form['filters'] = array(
    '#type' => 'fieldset',
    '#title' => t('Show only items where'),
    '#theme' => 'exposed_filters__node',
  );
  $existing_values = array();
  foreach ($session as $filter) {
    list($type, $value) = $filter;
    $existing_values[$type] = $value;
    if ($type == 'term') {
      // Load term name from DB rather than search and parse options array.
      $value = module_invoke('taxonomy', 'term_load', $value);
      $value = $value->name;
    }
    elseif ($type == 'language') {
      $value = $value == LANGUAGE_NONE ? t('Language neutral') :
        module_invoke('locale', 'language_name', $value);
    }
    else {
      $value = $filters[$type]['options'][$value];
    }
    $t_args = array('%property' => $filters[$type]['title'],
      '%value' => $value);
    if ($i++) {
      // $form['filters']['current'][] = array('#markup' =>
      // t('and where %property is %value', $t_args));
    }
    else {
      // $form['filters']['current'][] = array('#markup' =>
      // t('where %property is %value', $t_args));
    }
    if (in_array($type, array('type', 'language'))) {
      // Remove the option if it is already being filtered on.
      // unset($filters[$type]);
    }
  }

  $form['filters']['status'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('clearfix')),
      /* '#prefix' => ($i ? '<div class="additional-filters">' .
       * t('and where') . '</div>' : ''), */
  );
  $form['filters']['status']['filters'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('filters')),
  );
  foreach ($filters as $key => $filter) {
    $form['filters']['status']['filters'][$key] = array(
      '#type' => 'select',
      '#options' => $filter['options'],
      '#title' => check_plain($filter['title']),
      '#default_value' => isset($existing_values[$key]) ?
          $existing_values[$key] : '[any]',
    );
  }

  $form['filters']['status']['actions'] = array(
    '#type' => 'actions',
    '#attributes' => array('class' => array('container-inline')),
  );
  $form['filters']['status']['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Filter'),
  );
  if (count($session)) {
    /* $form['filters']['status']['actions']['undo'] = array('#type' => 'submit'
     *   , '#value' => t('Undo')); */
    $form['filters']['status']['actions']['reset'] = array(
      '#type' => 'submit',
      '#value' => t('Reset')
    );
  }

  drupal_add_js('misc/form.js');

  return $form;
}

/**
 * Additional validation for uottawa_lang_editor_admin_nodes().
 *
 * Validates the extra select buttons for language-specific nodes.
 */
function uottawa_lang_editor_admin_nodes_validate($form, &$form_state) {
  $items = array();
  if (is_array($form_state['values']['nodes'])) {
    foreach ($form_state['values']['nodes'] as $key => $item) {
      $pieces = explode("_", $key);
      if ($item !== 0) {
        $items[$pieces[0]] = $pieces[0];
        if (isset($pieces[1])) {
          $items[$pieces[1]] = $pieces[1];
        }
      }
      else {
        if (isset($_POST['select_eng'][$pieces[0]]) &&
            $_POST['select_eng'][$pieces[0]] == '1') {
          $items[$pieces[0]] = $pieces[0];
        }
        if (isset($pieces[1])) {
          if (isset($_POST['select_eng'][$pieces[1]]) &&
              $_POST['select_eng'][$pieces[1]] == '1') {
            $items[$pieces[1]] = $pieces[1];
          }
        }
        if (isset($_POST['select_fra'][$pieces[0]]) &&
            $_POST['select_fra'][$pieces[0]] == '1') {
          $items[$pieces[0]] = $pieces[0];
        }
        if (isset($pieces[1])) {
          if (isset($_POST['select_fra'][$pieces[1]]) &&
              $_POST['select_fra'][$pieces[1]] == '1') {
            $items[$pieces[1]] = $pieces[1];
          }
        }
      }
    }
  }
  $form_state['values']['nodes'] = $items;
}

/**
 * Override of node_admin_nodes().
 *
 * Tweaked to provide side-by-side view of English/French content.
 */
function uottawa_lang_editor_admin_nodes() {
  $form = array();
  $admin_access = user_access('administer nodes');

  // Build the 'Update options' form.
  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update options'),
    '#attributes' => array('class' => array('container-inline')),
    '#access' => $admin_access,
  );
  $options = array();
  foreach (module_invoke_all('node_operations') as $operation => $array) {
    $options[$operation] = $array['label'];
  }
  $form['options']['operation'] = array(
    '#type' => 'select',
    '#title' => t('Operation'),
    '#title_display' => 'invisible',
    '#options' => $options,
    '#default_value' => 'approve',
  );
  $form['options']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
    '#validate' => array('uottawa_lang_editor_admin_nodes_validate'),
    '#submit' => array('node_admin_nodes_submit'),
  );

  // Build the sortable table header.
  $header = array(
    'select_eng' => array('data' => t('EN')),
    'title' => array(
      'data' => t('English Title'),
      'field' => 'n.title',
      'style' => 'width: 250px;',
    ),
    'select_fra' => array('data' => t('FR')),
    'fra_title' => array(
      'data' => t('French Title'),
      'field' => 'n2.title',
      'style' => 'width: 250px;',
    ),
    'type' => array('data' => t('Type'), 'field' => 'n.type'),
    'status' => array('data' => t('Status'), 'field' => 'n.status'),
    'changed' => array(
      'data' => t('Updated'),
      'field' => 'n.changed',
      'sort' => 'desc',
    ),
  );
  $header['operations'] = array('data' => t('Operations'));

  $query = db_select('node', 'n')->extend('PagerDefault')->extend('TableSort');
  node_build_filter_query($query);

  if (!user_access('bypass node access')) {
    // If the user is able to view their own unpublished nodes, allow them
    // to see these in addition to published nodes. Check that they actually
    // have some unpublished nodes to view before adding the condition.
    if (user_access('view own unpublished content') &&
        $own_unpublished = db_query('
          SELECT nid
          FROM {node}
          WHERE uid = :uid AND status = :status
        ', array(
          ':uid' => $GLOBALS['user']->uid,
          ':status' => 0
        ))->fetchCol()) {
      $query->condition(db_or()
              ->condition('n.status', 1)
              ->condition('n.nid', $own_unpublished, 'IN')
      );
    }
    else {
      // If not, restrict the query to published nodes.
      $query->condition('n.status', 1);
    }
  }
  // @todo note that this only works for two languages, will give odd results
  // for more than two languages
  $query->leftJoin('node', 'n2',
      'n2.tnid = n.nid AND n2.language != n.language');

  $nids = $query
      ->fields('n', array('nid'))
      ->limit(50)
      ->orderByHeader($header)
      ->addTag('node_access')
      ->execute()
      ->fetchCol();

  $nodes = node_load_multiple($nids);

  // Prepare the list of nodes.
  $languages = language_list();
  $destination = drupal_get_destination();
  $options = array();
  $ids_loaded = array();
  foreach ($nodes as $node) {
    if (!in_array($node->nid, $ids_loaded)) {
      $ids_loaded[] = $node->nid;
      $key = $node->nid;
      $french_node = NULL;
      $erase_eng_title = FALSE;
      if ($node->language == 'en') {
        $tnodes = translation_node_get_translations($node->tnid);
        if (isset($tnodes['fr'])) {
          $french_node = node_load($tnodes['fr']->nid);
          $ids_loaded[] = $french_node->nid;
          $key .= "_" . $french_node->nid;
        }
      }
      elseif ($node->language == 'fr') {
        $french_node = $node;
        $tnodes = translation_node_get_translations($node->tnid);
        if (isset($tnodes['en'])) {
          $node = node_load($tnodes['en']->nid);
          $ids_loaded[] = $node->nid;
          $key .= "_" . $node->nid;
        }
        else {
          $erase_eng_title = TRUE;
        }
      }
      $l_options = $node->language != LANGUAGE_NONE &&
          isset($languages[$node->language]) ?
              array('language' => $languages[$node->language]) : array();
      $options[$key] = array(
        'title' => array(
          'data' => array(
            '#type' => 'link',
            '#title' => $node->title,
            '#href' => 'node/' . $node->nid,
            '#options' => $l_options,
            '#suffix' => ' ' . theme('mark', array(
              'type' => node_mark($node->nid, $node->changed)
            )),
          ),
        ),
        'type' => check_plain(node_type_get_name($node)),
        'author' => theme('username', array('account' => $node)),
        'status' => $node->status ? t('published') : t('not published'),
        'changed' => format_date($node->changed, 'short'),
        'select_eng' => '',
        'select_fra' => '',
      );
      if (!empty($french_node)) {
        if ($erase_eng_title) {
          $options[$key]['title']['data'] = '<em>' . t('No node found') .
              '</em>';
        }
        $status = $french_node->status ? t('published') : t('not published');
        if ($status != $options[$key]['status']) {
          $options[$key]['status'] .= ' (en)<br />' . $status . ' (fr)';
        }
        $updated = format_date($french_node->changed, 'short');
        if ($updated != $options[$key]['changed']) {
          $options[$key]['changed'] .= ' (en)<br />' . $updated . ' (fr)';
        }
        $options[$key]['fra_title'] = array(
          'data' => array(
            '#type' => 'link',
            '#title' => $french_node->title,
            '#href' => 'node/' . $french_node->nid,
            '#options' => array('language' => $languages[$french_node->language]),
            '#suffix' => ' ' . theme('mark', array(
              'type' => node_mark($french_node->nid, $french_node->changed)
            )),
          )
        );
        if (!$erase_eng_title) {
          $options[$key]['select_eng'] = array(
            'data' => array(
              '#type' => 'checkbox',
              '#title' => '',
              '#attributes' => array(
                'name' => 'select_eng[' . $node->nid . ']'
              )
            )
          );
          $options[$key]['select_fra'] = array(
            'data' => array(
              '#type' => 'checkbox',
              '#title' => '',
              '#attributes' => array(
                'name' => 'select_fra[' . $french_node->nid . ']'
              )
            )
          );
        }
      }
      else {
        $options[$key]['fra_title']['data'] = '<em>' . t('No node found')
            . '</em>';
      }
      // Build a list of all the accessible operations for the current node.
      $operations = array();
      if (node_access('update', $node)) {
        $operations['edit'] = array(
          'title' => t('edit'),
          'href' => 'node/' . $node->nid . '/edit',
          'query' => $destination,
        );
      }
      $options[$key]['operations'] = array();
      if (count($operations) > 1) {
        // Render an unordered list of operations links.
        $options[$key]['operations'] = array(
          'data' => array(
            '#theme' => 'links__node_operations',
            '#links' => $operations,
            '#attributes' => array('class' => array('links', 'inline')),
          ),
        );
      }
      elseif (!empty($operations)) {
        // Render the first and only operation as a link.
        $link = reset($operations);
        $options[$key]['operations'] = array(
          'data' => array(
            '#type' => 'link',
            '#title' => $link['title'],
            '#href' => $link['href'],
            '#options' => array('query' => $link['query']),
          ),
        );
      }
    }
  }

  // Only use a tableselect when the current user is able to perform any
  // operations.
  if ($admin_access) {
    $form['nodes'] = array(
      '#type' => 'tableselect',
      '#header' => $header,
      '#options' => $options,
      '#empty_value' => t('No nodes found'),
    );
  }
  // Otherwise, use a simple table.
  else {
    $form['nodes'] = array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $options,
      '#empty' => t('No content available.'),
    );
  }

  $form['pager'] = array('#markup' => theme('pager'));
  return $form;
}


/**
 * Form for multiple deletes.
 */
function uottawa_lang_editor_node_multiple_delete_confirm($form, &$form_state, $nodes) {
  $form['nodes'] = array(
    '#prefix' => '<ul>',
    '#suffix' => '</ul>',
    '#tree' => TRUE
  );
  // array_filter returns only elements with TRUE values
  foreach ($nodes as $nid => $value) {
    $title = db_query('SELECT title FROM {node} WHERE nid = :nid', array(
      ':nid' => $nid
    ))->fetchField();
    $form['nodes'][$nid] = array(
      '#type' => 'hidden',
      '#value' => $nid,
      '#prefix' => '<li>',
      '#suffix' => check_plain($title) . "</li>\n",
    );
  }
  $form['operation'] = array('#type' => 'hidden', '#value' => 'delete');
  $form['#submit'][] =
      'uottawa_lang_editor_node_multiple_delete_confirm_submit';
  $confirm_question = format_plural(
      count($nodes),
      'Are you sure you want to delete this item?',
      'Are you sure you want to delete these items?'
  );
  return confirm_form(
      $form,
      $confirm_question,
      'admin/content',
      t('This action cannot be undone.'),
      t('Delete'),
      t('Cancel')
  );
}

/**
 * Submission handler for multiple node deletion.
 */
function uottawa_lang_editor_node_multiple_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    // delete multiple
    node_delete_multiple(array_keys($form_state['values']['nodes']));
    $count = count($form_state['values']['nodes']);
    watchdog('content', 'Deleted @count posts.', array('@count' => $count));
    drupal_set_message(format_plural($count, 'Deleted 1 post.',
        'Deleted @count posts.'));
  }
  // redirect
  $form_state['redirect'] = 'admin/content';
}

/**
 * Confirmation form for deletion.
 */
function uottawa_lang_editor_dual_language_delete_confirmation_form($form, &$form_state, $node_list) {
  $node_list = explode("-", $node_list);
  $actual_list = array();
  foreach ($node_list as $nid) {
    $actual_list[$nid] = TRUE;
  }
  module_load_include("inc", "node", "node.pages");
  return uottawa_lang_editor_node_multiple_delete_confirm($form, $form_state,
      $actual_list);
}
