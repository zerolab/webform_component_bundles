<?php

/**
 * @file
 * Theme functions.
 *
 * vim:set sw=2 ts=2 et ft=php:
 */

/**
 * Theme the bundle components form. Uses a table to organize the components.
 *
 * @see theme_webform_components_form()
 */
function theme_webform_component_bundles_components_form($variables) {
  $form = $variables['form'];

  $form['components']['#attached']['library'][] = array('webform', 'admin');

  // TODO: Attach these. See http://drupal.org/node/732022.
  drupal_add_tabledrag('webform-components', 'order', 'sibling', 'webform-weight');
  drupal_add_tabledrag('webform-components', 'match', 'parent', 'webform-pid', 'webform-pid', 'webform-cid');

  $bundle = $form['#bundle'];

  $header = array(
    t('Label'), t('Type'), t('Value'), t('Mandatory'), t('Weight'), array(
      'data' => t('Operations'),
      'colspan' => 3,
    ),
  );
  $rows = array();

  // Add a row containing form elements for a new item.
  unset($form['add']['name']['#title'], $form['add_type']['#description']);
  $form['add']['name']['#attributes']['rel'] = t('New component name');
  $form['add']['name']['#attributes']['class'] = array('webform-default-value');
  $form['add']['cid']['#attributes']['class'] = array('webform-cid');
  $form['add']['pid']['#attributes']['class'] = array('webform-pid');
  $form['add']['weight']['#attributes']['class'] = array('webform-weight');
  $row_data = array(
    drupal_render($form['add']['name']),
    drupal_render($form['add']['type']),
    '',
    drupal_render($form['add']['mandatory']),
    drupal_render($form['add']['cid']) . drupal_render($form['add']['pid']) . drupal_render($form['add']['weight']),
    array('colspan' => 3, 'data' => drupal_render($form['add']['add'])),
  );
  $add_form = array(
    'data' => $row_data,
    'class' => array('draggable', 'webform-add-form'),
  );

  $form_rendered = FALSE;

  if (!empty($bundle->components)) {
    $component_tree = array();
    $page_count = 1;
    _webform_components_tree_build($bundle->components, $component_tree, 0, $page_count);
    $component_tree = _webform_components_tree_sort($component_tree);
    if (!empty($component_tree)) {
      // Build the table rows.
      foreach ($component_tree['children'] as $cid => $component) {
        _webform_components_bundles_components_form_rows($bundle, $cid, $component, 0, $form, $rows, $add_form);
      }
    }
  }
  else {
    $rows[] = array(array('data' => t('No Components, add a component below.'), 'colspan' => 9));
  }

  // Append the add form if not already printed.
  if ($add_form) {
    $rows[] = $add_form;
  }

  $output = '';
  $output .= theme('table', array(
    'header' => $header,
    'rows' => $rows,
    'attributes' => array('id' => 'webform-components'),
  ));
  $output .= drupal_render_children($form);

  return $output;
}

/**
 * Helper for component rows.
 */
function _webform_components_bundles_components_form_rows($bundle, $cid, $component, $level, &$form, &$rows, &$add_form) {
  // Create presentable values.
  if (drupal_strlen($component['value']) > 30) {
    $component['value'] = drupal_substr($component['value'], 0, 30);
    $component['value'] .= '...';
  }
  $component['value'] = check_plain($component['value']);

  // Remove individual titles from the mandatory and weight fields.
  unset($form['components'][$cid]['mandatory']['#title']);
  unset($form['components'][$cid]['pid']['#title']);
  unset($form['components'][$cid]['weight']['#title']);

  // Add special classes for weight and parent fields.
  $form['components'][$cid]['cid']['#attributes']['class'] = array('webform-cid');
  $form['components'][$cid]['pid']['#attributes']['class'] = array('webform-pid');
  $form['components'][$cid]['weight']['#attributes']['class'] = array('webform-weight');

  // Build indentation for this row.
  $indents = '';
  for ($n = 1; $n <= $level; $n++) {
    $indents .= '<div class="indentation">&nbsp;</div>';
  }

  // Add each component to a table row.
  $row_data = array(
    $indents . filter_xss($component['name']),
    $form['add']['type']['#options'][$component['type']],
    ($component['value'] == '') ? '-' : $component['value'],
    drupal_render($form['components'][$cid]['mandatory']),
    drupal_render($form['components'][$cid]['cid']) . drupal_render($form['components'][$cid]['pid']) . drupal_render($form['components'][$cid]['weight']),
    l(t('Edit'), 'admin/content/webform-bundle/' . $bundle->bid . '/components/' . $cid),
    l(t('Clone'), 'admin/content/webform-bundle/' . $bundle->bid . '/components/' . $cid . '/clone'),
    l(t('Delete'), 'admin/content/webform-bundle/' . $bundle->bid . '/components/' . $cid . '/delete'),
  );
  $row_class = array('draggable');
  if (!webform_component_feature($component['type'], 'group')) {
    $row_class[] = 'tabledrag-leaf';
  }
  if ($component['type'] == 'pagebreak') {
    $row_class[] = 'tabledrag-root';
    $row_class[] = 'webform-pagebreak';
    $row_data[0] = array('class' => array('webform-pagebreak'), 'data' => $row_data[0]);
  }
  $rows[] = array('data' => $row_data, 'class' => $row_class);
  if (isset($component['children']) && is_array($component['children'])) {
    foreach ($component['children'] as $cid => $component) {
      _webform_components_bundles_components_form_rows($bundle, $cid, $component, $level + 1, $form, $rows, $add_form);
    }
  }

  // Add the add form if this was the last edited component.
  if (isset($_GET['cid']) && $component['cid'] == $_GET['cid'] && $add_form) {
    $add_form['data'][0] = $indents . $add_form['data'][0];
    $rows[] = $add_form;
    $add_form = FALSE;
  }
}
